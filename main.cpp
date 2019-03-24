#include "stdio.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <random>
#include <thread>
#include <atomic>
#include <string>
#include <chrono>

#define VERIFY_RESULT() 1 // verifies that the search functions got the right answer. prints out a message if they didn't.
#define MAKE_CSVS() 1 // the main test

static const size_t c_maxValue = 2000;           // the sorted arrays will have values between 0 and this number in them (inclusive)
static const size_t c_maxValueIncrement = 1;      // how much to increase our max value by with each step
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

    static std::random_device rd;
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
        float y = log1p(x) / maxValue;
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

struct Point
{
    Point(float xin, float yin) : x(xin), y(yin) {}
    Point(size_t xin, size_t yin) : x(float(xin)), y(float(yin)) {}
    float x;
    float y;
};
struct LinearEquation
{
    float m;
    float b;
};
bool GetLinearEqn(Point a, Point b, LinearEquation& result)
{
    if (a.x > b.x)
        std::swap(a,b);

    if (b.x - a.x == 0)
        return false;
    result.m = (b.y - a.y) / (b.x - a.x);
    result.b = a.y - result.m * a.x;

    return true;
}

TestResults TestList_Gradient(const std::vector<size_t>& values, size_t searchValue)
{
    // The idea of this test is somewhat similar to that of TestList_LineFit.
    // Instead of assuming that our data fits a linear line between our min
    // and max, we sample around min and max (1 point near each) to get the
    // local gradient. Once we have that, we calculate a linear derivative of
    // the line that approximates the endpoints' locations and the tangent
    // line at each. From there, we propagate up y-intercept points and plug
    // them into the inverse function of our line

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

    // Calculate an approximation line
    // Assume y'' = c1
    // y' = xc1 + c2
    // y = x^2/2 * c1 + xc2 + c3
    // 0 = x^2/2 * c1 + xc2 + (c3 - y)
    // x = (-c2 +- sqrt(c2 * c2 - 2 * c1 * (c3 - y))) / c1

    // tan1 = tangent to min, tan2 = tangent to max, prime = y'
    LinearEquation tan1, tan2;
    LinearEquation prime;
    
    auto updateEquations = [&]() -> bool
    {
        // Update tan1, tan2
        const size_t offset = (maxIndex - minIndex) / 10; // No good reason for choosing 10. There are probably better values
        // const size_t offset = 10; // Can also try an absolute offset 
        if (offset == 0 || offset > (maxIndex - minIndex))
            return false;

        if (!GetLinearEqn({minIndex, values[minIndex]}, {minIndex + offset, values[minIndex + offset]}, tan1))
            return false;

        if (!GetLinearEqn({maxIndex, values[maxIndex]}, {maxIndex - offset, values[maxIndex - offset]}, tan2))
            return false;

        // Update y'
        // prime.m = c1
        // prime.b = c2
        if (!GetLinearEqn({float(minIndex), tan1.m}, {float(maxIndex), tan2.m}, prime))
            return false;

        return true;
    };

    auto getGuess = [&](size_t y, size_t& x) -> bool
    {
        // Solve for c3 using min
        const float c3 = values[minIndex] - minIndex * minIndex / 2.0f * prime.m - minIndex * prime.b;

        // Solve for x.
        // y = x^2/2 * c1 + xc2 + c3
        // 0 = x^2/2 * c1 + xc2 + (c3 - y)
        // x = (-c2 +- sqrt(c2 * c2 - 2 * c1 * (c3 - y))) / c1
        const float c1 = prime.m;
        const float c2 = prime.b;

        if ( c2 * c2 - 2 * c1 * (c3 - y) < 0.0f )
        {
            return false;
        }

        if ( c1 == 0.0f )
        {
            return false;
        }

        const float x1f = (-c2 + std::sqrt(c2 * c2 - 2 * c1 * (c3 - y))) / c1;
        const float x2f = (-c2 - std::sqrt(c2 * c2 - 2 * c1 * (c3 - y))) / c1;

        const size_t x1 = size_t(x1f + 0.5f);
        const size_t x2 = size_t(x2f + 0.5f);

        const bool valid1 = x1 > minIndex && x1 < maxIndex;
        const bool valid2 = x2 > minIndex && x2 < maxIndex;
        if (!valid1 && !valid2)
        {
            return false;
        }
        else if(valid1 && !valid2)
        {
            x = x1;
            return true;
        }
        else if(!valid1 && valid2)
        {
            x = x2;
            return true;
        }

        // Both x1 and x2 are valid and in range
        // If we're concave up, choose the greater, concave down the lesser
        // This works because we know y' is positive
        if (c1 > 0)
        {
            x = std::max(x1, x2);
            return true;
        }
        else
        {
            x = std::min(x1, x2);
            return true;
        }
    };

    bool validEquations = updateEquations();

    while (1)
    {
        // make a guess based on our line fit
        ret.guesses++;
        size_t guessIndex;
        if ( !validEquations || !getGuess(searchValue, guessIndex) )
        {
            // Fall back to binary search
            guessIndex = (minIndex + maxIndex) / 2;
        }

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

        validEquations = updateEquations();
    }

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
        {"Gradient", TestList_Gradient},
    };

#if MAKE_CSVS()

    size_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    threads.resize(numThreads);

    // To store off results
    std::vector<std::string> columns =
    { "DataDist", "SearchFn", "Samples", "Min", "Max", "Avg", "Single" };
    struct ResultRow {
        size_t dataDistIdx;
        size_t searchFnIdx;
        size_t samples;
        size_t minimum;
        size_t maximum;
        float avg;
        size_t single;
    };

    // Every group is owned by 1 thread.
    std::vector<ResultRow> results[countof(MakeFns)];

    // We store off the actual values for each distribution
    std::vector<size_t> samples[countof(MakeFns)];

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

                    static std::random_device rd;
                    static std::seed_seq fullSeed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
                    static std::mt19937 rng(fullSeed);

                    // for each test
                    std::vector<size_t> values;
                    for (size_t testIndex = 0; testIndex < countof(TestFns); ++testIndex)
                    {
                        // for each result
                        for (size_t numValues = 1; numValues <= c_maxNumValues; numValues += c_maxValueIncrement)
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

                            ResultRow result;
                            result.samples = numValues;
                            result.dataDistIdx = makeIndex;
                            result.searchFnIdx = testIndex;
                            result.minimum = guessMin;
                            result.maximum = guessMax;
                            result.avg = guessAverage;
                            result.single = guessSingle;
                            results[makeIndex].emplace_back(std::move(result));
                        }
                    }

                    samples[makeIndex] = std::move(values);

                    printf("Done with %s\n", MakeFns[makeIndex].name);

                    makeIndex = nextRow.fetch_add(1);
                }
            }
        );
    }

    for (std::thread& t : threads)
        t.join();

    // Write output to files
    printf("Writing result output\n");

    {
        std::ofstream out("results.csv", std::ios::trunc | std::ios::out);
#if WRITE_CSV_HEADERS
        // Header
        for (size_t i = 0; i < columns.size(); ++i)
        {
            out << '"' << columns[i] << '"';
            if (i != columns.size() - 1)
                out << ',';
        }
        out << std::endl;
#endif

        // Values
        for (const std::vector<ResultRow>& group : results)
        {
            for (const ResultRow& row : group)
            {
                out << '"' << MakeFns[row.dataDistIdx].name << "\","
                    << '"' << TestFns[row.searchFnIdx].name << "\","
                    << row.samples << ','
                    << row.minimum << ','
                    << row.maximum << ','
                    << row.avg << ','
                    << row.single << std::endl;
            }
        }
    }

    // Write samples to files
    printf("Writing samples\n");
    {
        std::ofstream out("samples.csv", std::ios::trunc | std::ios::out);
#if WRITE_CSV_HEADERS
        out << "\"Dist\",\"Index\",\"Value\"" << std::endl;
#endif

        for (size_t i = 0; i < countof(samples); ++i)
        {
            const std::vector<size_t>& sample = samples[i];
            for (size_t j = 0; j < sample.size(); ++j)
            {
                out << '"' << MakeFns[i].name << "\","
                    << j << ',' << sample[j] << std::endl;
            }
        }
    }

#endif // MAKE_CSVS()

    // Do perf tests
    {
        static std::random_device rd;
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

#ifdef _WIN32
    system("pause");
#endif

    return 0;
}
