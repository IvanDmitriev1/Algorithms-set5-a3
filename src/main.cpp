#include "HyperLogLog.h"
#include "RandomStreamGen.h"
#include "Hashers.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <print>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

constexpr size_t kStreamCount = 5;
constexpr size_t kStreamSize = 120'000;
constexpr size_t kStepPercent = 5;
constexpr uint64_t kSeedBase = 0xC0FFEE1234ULL;
constexpr uint32_t kHashSeed32 = 0x9e3779b1u;

constexpr uint32_t kPolyBase = 12u;
constexpr uint32_t kFnvBase = 16u;
constexpr std::array<uint8_t, 5> kPrecisionBitsSet = { 6, 8, 10, 12, 14 };

struct SummaryRow
{
    std::string hasherName;
    uint8_t b = 0;
    size_t m = 0;
    double theory104 = 0.0;
    double theory13 = 0.0;
    double meanAbsRelErr = 0.0;
    double maxAbsRelErr = 0.0;
    double meanSigmaOverE = 0.0;
    double maxSigmaOverE = 0.0;
    double shareSigmaLe104 = 0.0;
    double shareSigmaLe13 = 0.0;
};

double Mean(const std::vector<double>& values)
{
    if (values.empty())
    {
        return 0.0;
    }

    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / static_cast<double>(values.size());
}

double SampleStdDev(const std::vector<double>& values, double meanValue)
{
    if (values.size() < 2)
    {
        return 0.0;
    }

    double sqDiffSum = 0.0;
    for (double value : values)
    {
        const double diff = value - meanValue;
        sqDiffSum += diff * diff;
    }

    return std::sqrt(sqDiffSum / static_cast<double>(values.size() - 1));
}

template <typename THasher>
void CollectStreamSeries(
    const RandomStreamGen& stream,
    const THasher& hasher,
    uint8_t precisionBits,
    std::vector<size_t>& stepSizes,
    std::vector<double>& exactFt0,
    std::vector<double>& estimateNt)
{
    HyperLogLog<THasher> hll(precisionBits, hasher);
    std::unordered_set<std::string_view> exactSeen;
    exactSeen.reserve(stream.Size());

    const auto fullStream = stream.Prefix(stream.Size());
    stepSizes = stream.PrefixSizesByStepPercent(kStepPercent);

    exactFt0.clear();
    estimateNt.clear();
    exactFt0.reserve(stepSizes.size());
    estimateNt.reserve(stepSizes.size());

    size_t stepIndex = 0;
    while (stepIndex < stepSizes.size() && stepSizes[stepIndex] == 0)
    {
        exactFt0.push_back(0.0);
        estimateNt.push_back(hll.Estimate());
        ++stepIndex;
    }

    for (size_t i = 0; i < fullStream.size() && stepIndex < stepSizes.size(); ++i)
    {
        const std::string_view value = fullStream[i];
        hll.Add(value);
        exactSeen.insert(value);

        const size_t processedCount = i + 1;
        while (stepIndex < stepSizes.size() && processedCount >= stepSizes[stepIndex])
        {
            exactFt0.push_back(static_cast<double>(exactSeen.size()));
            estimateNt.push_back(hll.Estimate());
            ++stepIndex;
        }
    }

    while (stepIndex < stepSizes.size())
    {
        exactFt0.push_back(static_cast<double>(exactSeen.size()));
        estimateNt.push_back(hll.Estimate());
        ++stepIndex;
    }
}

SummaryRow WriteSeriesAndBuildSummary(
    const std::filesystem::path& path,
    const std::string& hasherName,
    uint8_t precisionBits,
    const std::vector<size_t>& stepSizes,
    const std::vector<double>& firstFt0,
    const std::vector<double>& firstNt,
    const std::vector<std::vector<double>>& allNt)
{
    SummaryRow summary;
    summary.hasherName = hasherName;
    summary.b = precisionBits;
    summary.m = size_t{ 1 } << precisionBits;
    summary.theory104 = 1.04 / std::sqrt(static_cast<double>(summary.m));
    summary.theory13 = 1.3 / std::sqrt(static_cast<double>(summary.m));

    if (allNt.empty())
    {
        return summary;
    }

    size_t stepCount = std::min({ stepSizes.size(), firstFt0.size(), firstNt.size() });
    for (const auto& ntSeries : allNt)
    {
        stepCount = std::min(stepCount, ntSeries.size());
    }

    std::ofstream out(path);
    out << "step_index,prefix_size,ft0_exact,nt_estimate,mean_nt,std_nt,lower_nt,upper_nt\n";
    out << std::fixed << std::setprecision(6);

    std::vector<double> ntValues;
    ntValues.reserve(allNt.size());

    double sumAbsRelErr = 0.0;
    double maxAbsRelErr = 0.0;
    double sumSigmaOverE = 0.0;
    double maxSigmaOverE = 0.0;
    size_t sigmaCount = 0;
    size_t errCount = 0;
    size_t sigmaLe104Count = 0;
    size_t sigmaLe13Count = 0;

    for (size_t step = 0; step < stepCount; ++step)
    {
        ntValues.clear();
        for (const auto& ntSeries : allNt)
        {
            ntValues.push_back(ntSeries[step]);
        }

        const double meanNt = Mean(ntValues);
        const double stdNt = SampleStdDev(ntValues, meanNt);
        const double ft0 = firstFt0[step];
        const double sigmaOverE = meanNt > 0.0 ? (stdNt / meanNt) : 0.0;

        out << step << ','
            << stepSizes[step] << ','
            << ft0 << ','
            << firstNt[step] << ','
            << meanNt << ','
            << stdNt << ','
            << (meanNt - stdNt) << ','
            << (meanNt + stdNt) << '\n';

        if (ft0 > 0.0)
        {
            const double absRelErr = std::abs(meanNt - ft0) / ft0;
            sumAbsRelErr += absRelErr;
            maxAbsRelErr = std::max(maxAbsRelErr, absRelErr);
            ++errCount;
        }

        if (meanNt > 0.0)
        {
            sumSigmaOverE += sigmaOverE;
            maxSigmaOverE = std::max(maxSigmaOverE, sigmaOverE);
            sigmaLe104Count += static_cast<size_t>(sigmaOverE <= summary.theory104);
            sigmaLe13Count += static_cast<size_t>(sigmaOverE <= summary.theory13);
            ++sigmaCount;
        }
    }

    summary.meanAbsRelErr = errCount > 0 ? sumAbsRelErr / static_cast<double>(errCount) : 0.0;
    summary.maxAbsRelErr = maxAbsRelErr;
    summary.meanSigmaOverE = sigmaCount > 0 ? sumSigmaOverE / static_cast<double>(sigmaCount) : 0.0;
    summary.maxSigmaOverE = maxSigmaOverE;
    summary.shareSigmaLe104 = sigmaCount > 0 ? static_cast<double>(sigmaLe104Count) / static_cast<double>(sigmaCount) : 0.0;
    summary.shareSigmaLe13 = sigmaCount > 0 ? static_cast<double>(sigmaLe13Count) / static_cast<double>(sigmaCount) : 0.0;

    return summary;
}

void WriteSummaryCsv(const std::filesystem::path& path, const std::vector<SummaryRow>& summaries)
{
    std::ofstream out(path);
    out << "hasher,b,m,theory_104,theory_13,mean_abs_rel_err,max_abs_rel_err,mean_sigma_over_e,max_sigma_over_e,share_sigma_le_104,share_sigma_le_13\n";
    out << std::fixed << std::setprecision(6);

    for (const SummaryRow& row : summaries)
    {
        out << row.hasherName << ','
            << static_cast<int>(row.b) << ','
            << row.m << ','
            << row.theory104 << ','
            << row.theory13 << ','
            << row.meanAbsRelErr << ','
            << row.maxAbsRelErr << ','
            << row.meanSigmaOverE << ','
            << row.maxSigmaOverE << ','
            << row.shareSigmaLe104 << ','
            << row.shareSigmaLe13 << '\n';
    }
}

template <typename THasher>
SummaryRow RunExperimentForB(
    const std::string& hasherName,
    const THasher& hasher,
    uint8_t precisionBits,
    const std::filesystem::path& outputDir)
{
    std::vector<size_t> stepSizes;
    std::vector<double> firstFt0;
    std::vector<double> firstNt;
    std::vector<std::vector<double>> allNt;
    allNt.reserve(kStreamCount);

    for (size_t streamIndex = 0; streamIndex < kStreamCount; ++streamIndex)
    {
        RandomStreamGen stream(kSeedBase + streamIndex + 1, kStreamSize);

        std::vector<size_t> currentStepSizes;
        std::vector<double> currentFt0;
        std::vector<double> currentNt;
        CollectStreamSeries(stream, hasher, precisionBits, currentStepSizes, currentFt0, currentNt);

        if (streamIndex == 0)
        {
            stepSizes = std::move(currentStepSizes);
            firstFt0 = std::move(currentFt0);
            firstNt = currentNt;
        }

        allNt.push_back(std::move(currentNt));
    }

    const std::string fileName = hasherName + "_B" + std::to_string(static_cast<int>(precisionBits)) + ".csv";
    return WriteSeriesAndBuildSummary(outputDir / fileName, hasherName, precisionBits, stepSizes, firstFt0, firstNt, allNt);
}

int main()
{
    std::println("Running HyperLogLog...");

    const std::filesystem::path outputDir = "output";
    std::filesystem::create_directories(outputDir);

    for (const auto& entry : std::filesystem::directory_iterator(outputDir))
    {
        if (entry.path().extension() == ".csv")
        {
            std::filesystem::remove(entry.path());
        }
    }

    std::vector<SummaryRow> summaries;
    summaries.reserve(kPrecisionBitsSet.size() * 2);

    for (uint8_t b : kPrecisionBitsSet)
    {
        summaries.push_back(RunExperimentForB(
            "PolyHash32",
            HashFuncGen::MakePolyHash32(kPolyBase, kHashSeed32),
            b,
            outputDir));

        summaries.push_back(RunExperimentForB(
            "Fnv1a32",
            HashFuncGen::MakeFnv1a32(kFnvBase, kHashSeed32),
            b,
            outputDir));
    }

    WriteSummaryCsv(outputDir / "b_analysis.csv", summaries);

    std::println("Saved files and output/b_analysis.csv.");
    std::println("Run plot_hll.py.");
    return 0;
}
