#include "stdio.h"
#include <vector>
#include <random>
#include <thread>
#include <atomic>
#include <string>
#include <chrono>

#define VERIFY_RESULT() 1 // verifies that the search functions got the right answer. prints out a message if they didn't.
#define MAKE_CSVS() 1 // the main test

static const size_t c_maxValue = 2000;           // the sorted arrays will have values between 0 and this number in them (inclusive)
static const size_t c_maxNumValues = 1000;       // the graphs will graph between 1 and this many values in a sorted array
static const size_t c_perfTestNumSearches = 100000; // how many searches are going to be done per list type, to come up with timing for a search type.
#if MAKE_CSVS()
static const size_t c_numRunsPerTest = 100;      // how many times does it do the same test to gather min, max, average?
#endif

struct TestResults
{
    bool found;
    size_t index;
    size_t guesses;
};

using MakeListFn = void(*)(std::vector<size_t>& values, size_t count);
using TestListFn = TestResults(*)(const std::vector<size_t>& values, size_t searchValue);

struct MakeListInfo
{
    const char* name;
    MakeListFn fn;
};

struct TestListInfo
{
    const char* name;
    TestListFn fn;
};

#define countof(array) (sizeof(array) / sizeof(array[0]))

template <typename T>
T Clamp(T min, T max, T value)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

float Lerp(float a, float b, float t)
{
    return (1.0f - t) * a + t * b;
}

// ------------------------ MAKE LIST FUNCTIONS ------------------------

void MakeList_Random(std::vector<size_t>& values, size_t count)
{
    std::uniform_int_distribution<size_t> dist(0, c_maxValue);

    static std::random_device rd("dev/random");
    static std::seed_seq fullSeed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
    static std::mt19937 rng(fullSeed);

    values.resize(count);
    for (size_t& v : values)
        v = dist(rng);

    std::sort(values.begin(), values.end());
}

void MakeList_Linear(std::vector<size_t>& values, size_t count)
{
    values.resize(count);
    for (size_t index = 0; index < count; ++index)
    {
        float x = float(index) / (count > 1 ? float(count - 1) : 1);
        float y = x;
        y *= c_maxValue;
        values[index] = size_t(y);
    }

    std::sort(values.begin(), values.end());
}

void MakeList_Linear_Outlier(std::vector<size_t>& values, size_t count)
{
    MakeList_Linear(values, count);
    *values.rbegin() = c_maxValue * 100;
}

void MakeList_Quadratic(std::vector<size_t>& values, size_t count)
{
    values.resize(count);
    for (size_t index = 0; index < count; ++index)
    {
        float x = float(index) / (count > 1 ? float(count - 1) : 1);
        float y = x * x;
        y *= c_maxValue;
        values[index] = size_t(y);
    }

    std::sort(values.begin(), values.end());
}

void MakeList_Cubic(std::vector<size_t>& values, size_t count)
{
    values.resize(count);
    for (size_t index = 0; index < count; ++index)
    {
        float x = float(index) / (count > 1 ? float(count-1) : 1);
        float y = x * x * x;
        y *= c_maxValue;
        values[index] = size_t(y);
    }

    std::sort(values.begin(), values.end());
}

void MakeList_Log(std::vector<size_t>& values, size_t count)
{
    values.resize(count);

    float maxValue = log(float(count));

    for (size_t index = 0; index < count; ++index)
    {
        float x = float(index + 1);
        float y = log(x+1) / maxValue;
        y *= c_maxValue;
        values[index] = size_t(y);
    }

    std::sort(values.begin(), values.end());
}

// ------------------------ TEST LIST FUNCTIONS ------------------------

TestResults TestList_LinearSearch(const std::vector<size_t>& values, size_t searchValue)
{
    TestResults ret;
    ret.found = false;
    ret.guesses = 0;
    ret.index = 0;

    while (1)
    {
        if (ret.index >= values.size())
            break;
        ret.guesses++;

        size_t value = values[ret.index];
        if (value == searchValue)
        {
            ret.found = true;
            break;
        }
        if (value > searchValue)
            break;

        ret.index++;
    }

    return ret;
}

TestResults TestList_LineFit(const std::vector<size_t>& values, size_t searchValue)
{
    // The idea of this test is that we keep a fit of a line y=mx+b
    // of the left and right side known data points, and use that
    // info to make a guess as to where the value will be.
    //
    // When a guess is wrong, it becomes the new left or right of the line
    // depending on if it was too low (left) or too high (right).
    //
    // This function returns how many steps it took to find the value
    // but doesn't include the min and max reads at the beginning because
    // those could reasonably be done in advance.

    // get the starting min and max value.
    size_t minIndex = 0;
    size_t maxIndex = values.size() - 1;
    size_t min = values[minIndex];
    size_t max = values[maxIndex];

    TestResults ret;
    ret.found = true;
    ret.guesses = 0;

    // if we've already found the value, we are done
    if (searchValue < min)
    {
        ret.index = minIndex;
        ret.found = false;
        return ret;
    }
    if (searchValue > max)
    {
        ret.index = maxIndex;
        ret.found = false;
        return ret;
    }
    if (searchValue == min)
    {
        ret.index = minIndex;
        return ret;
    }
    if (searchValue == max)
    {
        ret.index = maxIndex;
        return ret;
    }

    // fit a line to the end points
    // y = mx + b
    // m = rise / run
    // b = y - mx
    float m = (float(max) - float(min)) / float(maxIndex - minIndex);
    float b = float(min) - m * float(minIndex);

    while (1)
    {
        // make a guess based on our line fit
        ret.guesses++;
        size_t guessIndex = size_t(0.5f + (float(searchValue) - b) / m);
        guessIndex = Clamp(minIndex + 1, maxIndex - 1, guessIndex);
        size_t guess = values[guessIndex];

        // if we found it, return success
        if (guess == searchValue)
        {
            ret.index = guessIndex;
            return ret;
        }

        // if we were too low, this is our new minimum
        if (guess < searchValue)
        {
            minIndex = guessIndex;
            min = guess;
        }
        // else we were too high, this is our new maximum
        else
        {
            maxIndex = guessIndex;
            max = guess;
        }

        // if we run out of places to look, we didn't find it
        if (minIndex + 1 >= maxIndex)
        {
            ret.index = minIndex;
            ret.found = false;
            return ret;
        }

        // fit a new line
        m = (float(max) - float(min)) / float(maxIndex - minIndex);
        b = float(min) - m * float(minIndex);
    }

    return ret;
}

TestResults TestList_HybridSearch(const std::vector<size_t>& values, size_t searchValue)
{
    // On even iterations, this does a line fit step.
    // On odd iterations, this does a binary search step.
    // Line fit can do better than binary search, but it can also get trapped in situations that it does poorly.
    // The binary search step is there to help it break out of those situations.

    // get the starting min and max value.
    size_t minIndex = 0;
    size_t maxIndex = values.size() - 1;
    size_t min = values[minIndex];
    size_t max = values[maxIndex];

    TestResults ret;
    ret.found = true;
    ret.guesses = 0;

    // if we've already found the value, we are done
    if (searchValue < min)
    {
        ret.index = minIndex;
        ret.found = false;
        return ret;
    }
    if (searchValue > max)
    {
        ret.index = maxIndex;
        ret.found = false;
        return ret;
    }
    if (searchValue == min)
    {
        ret.index = minIndex;
        return ret;
    }
    if (searchValue == max)
    {
        ret.index = maxIndex;
        return ret;
    }

    // fit a line to the end points
    // y = mx + b
    // m = rise / run
    // b = y - mx
    float m = (float(max) - float(min)) / float(maxIndex - minIndex);
    float b = float(min) - m * float(minIndex);

    bool doBinaryStep = false;
    while (1)
    {
        // make a guess based on our line fit, or by binary search, depending on the value of doBinaryStep
        ret.guesses++;
        size_t guessIndex = doBinaryStep ? (minIndex + maxIndex) / 2 : size_t(0.5f + (float(searchValue) - b) / m);
        guessIndex = Clamp(minIndex + 1, maxIndex - 1, guessIndex);
        size_t guess = values[guessIndex];

        // if we found it, return success
        if (guess == searchValue)
        {
            ret.index = guessIndex;
            return ret;
        }

        // if we were too low, this is our new minimum
        if (guess < searchValue)
        {
            minIndex = guessIndex;
            min = guess;
        }
        // else we were too high, this is our new maximum
        else
        {
            maxIndex = guessIndex;
            max = guess;
        }

        // if we run out of places to look, we didn't find it
        if (minIndex + 1 >= maxIndex)
        {
            ret.index = minIndex;
            ret.found = false;
            return ret;
        }

        // fit a new line
        m = (float(max) - float(min)) / float(maxIndex - minIndex);
        b = float(min) - m * float(minIndex);

        // toggle what search mode we are using
        doBinaryStep = !doBinaryStep;
    }

    return ret;
}

TestResults TestList_BinarySearch(const std::vector<size_t>& values, size_t searchValue)
{
    TestResults ret;
    ret.found = false;
    ret.guesses = 0;

    size_t minIndex = 0;
    size_t maxIndex = values.size()-1;
    while (1)
    {
        // make a guess by looking in the middle of the unknown area
        ret.guesses++;
        size_t guessIndex = (minIndex + maxIndex) / 2;
        size_t guess = values[guessIndex];

        // found it
        if (guess == searchValue)
        {
            ret.found = true;
            ret.index = guessIndex;
            return ret;
        }
        // if our guess was too low, it's the new min
        else if (guess < searchValue)
        {
            minIndex = guessIndex + 1;
        }
        // if our guess was too high, it's the new max
        else if (guess > searchValue)
        {
            // underflow prevention
            if (guessIndex == 0)
            {
                ret.index = guessIndex;
                return ret;
            }
            maxIndex = guessIndex - 1;
        }

        // fail case
        if (minIndex > maxIndex)
        {
            ret.index = guessIndex;
            return ret;
        }
    }

    return ret;
}

TestResults TestList_LineFitBlind(const std::vector<size_t>& values, size_t searchValue)
{
    // If you want to know how this does against binary search without first knowing the min and max, this result is for you.
    // It takes 2 extra samples to get the min and max, so we are counting those as guesses (memory reads).
    TestResults ret = TestList_LineFit(values, searchValue);
    ret.guesses += 2;
    return ret;
}

// ------------------------ MAIN ------------------------

void VerifyResults(const std::vector<size_t>& values, size_t searchValue, const TestResults& result, const char* list, const char* test)
{
    #if VERIFY_RESULT()
    // verify correctness of result by comparing to a linear search
    TestResults actualResult = TestList_LinearSearch(values, searchValue);
    if (result.found != actualResult.found)
    {
        printf("VERIFICATION FAILURE!! (found %s vs %s) %s, %s\n", result.found ? "true" : "false", actualResult.found ? "true" : "false", list, test);
    }
    else if (result.found == true && result.index != actualResult.index && values[result.index] != values[actualResult.index])
    {
        // Note that in the case of duplicates, different algorithms may return different indices, but the values stored in them should be the same
        printf("VERIFICATION FAILURE!! (index %zu vs %zu) %s, %s\n", result.index, actualResult.index, list, test);
    }
    // verify that the index returned is a reasonable place for the value to be inserted, if the value was not found.
    else if (result.found == false)
    {
        bool gte = true;
        bool lte = true;

        if (result.index > 0)
            gte = searchValue >= values[result.index - 1];

        if (result.index + 1 < values.size())
            lte = searchValue <= values[result.index + 1];

        if (gte == false || lte == false)
            printf("VERIFICATION FAILURE!! Not a valid place to insert a new value! %s, %s\n", list, test);
    }

    #endif
}

int main(int argc, char** argv)
{
    MakeListInfo MakeFns[] =
    {
        {"Random", MakeList_Random},
        {"Linear", MakeList_Linear},
        {"Linear Outlier", MakeList_Linear_Outlier},
        {"Quadratic", MakeList_Quadratic},
        {"Cubic", MakeList_Cubic},
        {"Log", MakeList_Log},
    };

    TestListInfo TestFns[] =
    {
        {"Linear Search", TestList_LinearSearch},
        {"Line Fit", TestList_LineFit},
        {"Line Fit Blind", TestList_LineFitBlind},
        {"Binary Search", TestList_BinarySearch},
        {"Hybrid", TestList_HybridSearch},
    };

#if MAKE_CSVS()

    size_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    threads.resize(numThreads);

    typedef std::vector<std::string> TRow;
    typedef std::vector<TRow> TSheet;

    // for each numer sequence. Done multithreadedly
    std::atomic<size_t> nextRow(0);
    for (std::thread& t : threads)
    {
        t = std::thread(
            [&]()
            {
                size_t makeIndex = nextRow.fetch_add(1);
                while (makeIndex < countof(MakeFns))
                {
                    printf("Starting %s\n", MakeFns[makeIndex].name);

                    static std::random_device rd("dev/random");
                    static std::seed_seq fullSeed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
                    static std::mt19937 rng(fullSeed);

                    // the data to write to the csv file. a row per sample count plus one more for titles
                    TSheet csv;
                    csv.resize(c_maxNumValues + 1);

                    // make a column for the sample counts
                    char buffer[256];
                    csv[0].push_back("Sample Count");
                    for (size_t numValues = 1; numValues <= c_maxNumValues; ++numValues)
                    {
                        sprintf_s(buffer, "%zu", numValues);
                        csv[numValues].push_back(buffer);
                    }

                    // for each test
                    std::vector<size_t> values;
                    for (size_t testIndex = 0; testIndex < countof(TestFns); ++testIndex)
                    {
                        sprintf_s(buffer, "%s Min", TestFns[testIndex].name);
                        csv[0].push_back(buffer);
                        sprintf_s(buffer, "%s Max", TestFns[testIndex].name);
                        csv[0].push_back(buffer);
                        sprintf_s(buffer, "%s Avg", TestFns[testIndex].name);
                        csv[0].push_back(buffer);
                        sprintf_s(buffer, "%s Single", TestFns[testIndex].name);
                        csv[0].push_back(buffer);

                        // for each result
                        for (size_t numValues = 1; numValues <= c_maxNumValues; ++numValues)
                        {
                            size_t guessMin = ~size_t(0);
                            size_t guessMax = 0;
                            float guessAverage = 0.0f;
                            size_t guessSingle = 0;

                            // repeat it a number of times to gather min, max, average
                            for (size_t repeatIndex = 0; repeatIndex < c_numRunsPerTest; ++repeatIndex)
                            {
                                std::uniform_int_distribution<size_t> dist(0, c_maxValue);
                                size_t searchValue = dist(rng);

                                MakeFns[makeIndex].fn(values, numValues);
                                TestResults result = TestFns[testIndex].fn(values, searchValue);

                                VerifyResults(values, searchValue, result, MakeFns[makeIndex].name, TestFns[testIndex].name);

                                guessMin = std::min(guessMin, result.guesses);
                                guessMax = std::max(guessMax, result.guesses);
                                guessAverage = Lerp(guessAverage, float(result.guesses), 1.0f / float(repeatIndex + 1));
                                guessSingle = result.guesses;
                            }

                            sprintf_s(buffer, "%zu", guessMin);
                            csv[numValues].push_back(buffer);

                            sprintf_s(buffer, "%zu", guessMax);
                            csv[numValues].push_back(buffer);

                            sprintf_s(buffer, "%f", guessAverage);
                            csv[numValues].push_back(buffer);

                            sprintf_s(buffer, "%zu", guessSingle);
                            csv[numValues].push_back(buffer);
                        }
                    }

                    // make a column for the sampling sequence itself
                    csv[0].push_back("Sequence");
                    for (size_t numValues = 1; numValues <= c_maxNumValues; ++numValues)
                    {
                        sprintf_s(buffer, "%zu", values[numValues-1]);
                        csv[numValues].push_back(buffer);
                    }

                    char fileName[256];
                    sprintf_s(fileName, "out/%s.csv", MakeFns[makeIndex].name);
                    FILE* file = nullptr;
                    fopen_s(&file, fileName, "w+b");

                    for (const TRow& row : csv)
                    {
                        for (const std::string& cell : row)
                            fprintf(file, "\"%s\",", cell.c_str());
                        fprintf(file, "\n");
                    }

                    fclose(file);

                    printf("Done with %s\n", MakeFns[makeIndex].name);

                    makeIndex = nextRow.fetch_add(1);
                }
            }
        );
    }

    for (std::thread& t : threads)
        t.join();

#endif // MAKE_CSVS()

    // Do perf tests
    {
        static std::random_device rd("dev/random");
        static std::seed_seq fullSeed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
        static std::mt19937 rng(fullSeed);

        std::vector<size_t> values, searchValues;
        searchValues.resize(c_perfTestNumSearches);
        values.resize(c_maxNumValues);

        // make the search values that are going to be used by all the tests
        {
            std::uniform_int_distribution<size_t> dist(0, c_maxValue);
            for (size_t & v : searchValues)
                v = dist(rng);
        }

        // binary search, linear search, etc
        for (size_t testIndex = 0; testIndex < countof(TestFns); ++testIndex)
        {
            // quadratic numbers, random numbers, etc
            double timeTotal = 0.0f;
            size_t totalGuesses = 0;
            for (size_t makeIndex = 0; makeIndex < countof(MakeFns); ++makeIndex)
            {
                MakeFns[makeIndex].fn(values, c_maxNumValues);

                size_t guesses = 0;

                std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

                // do the searches
                for (size_t searchValue : searchValues)
                {
                    TestResults ret = TestFns[testIndex].fn(values, searchValue);
                    guesses += ret.guesses;
                    totalGuesses += ret.guesses;
                }

                std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

                std::chrono::duration<double> duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

                timeTotal += duration.count();
                printf("  %s %s : %f seconds\n", TestFns[testIndex].name, MakeFns[makeIndex].name, duration.count());
            }

            double timePerGuess = (timeTotal * 1000.0 * 1000.0 * 1000.0f) / double(totalGuesses);
            printf("%s total : %f seconds  (%zu guesses = %f nanoseconds per guess)\n\n", TestFns[testIndex].name, timeTotal, totalGuesses, timePerGuess);
        }
    }

    system("pause");

    return 0;
}
