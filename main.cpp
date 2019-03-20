#include "stdio.h"
#include <vector>
#include <random>
#include <thread>
#include <atomic>

static const size_t c_maxValue = 2000;      // the sorted arrays will have values between 0 and this number in them (inclusive)
static const size_t c_maxNumValues = 1000;  // the graphs will graph between 1 and this many values in a sorted array
static const size_t c_numRunsPerTest = 100; // how many times does it do the same test to gather min, max, average?

#define VERIFY_RESULT() 1 // verifies that the search functions got the right answer. prints out a message if they didn't.

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

size_t GetRandomIndex(const std::vector<size_t>& values)
{
    static std::random_device rd("dev/random");
    static std::seed_seq fullSeed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
    static std::mt19937 rng(fullSeed);
    std::uniform_int_distribution<size_t> dist(0, values.size() - 1);
    return dist(rng);
}

size_t GetRandomValue(const std::vector<size_t>& values)
{
    return values[GetRandomIndex(values)];
}

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
        float x = float(index) / float(count-1);
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
        float x = float(index) / float(count - 1);
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
        float x = float(index) / float(count-1);
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
    if (searchValue < min || searchValue > max)
    {
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
    if (searchValue < min || searchValue > max)
    {
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
                return ret;
            maxIndex = guessIndex - 1;
        }

        // fail case
        if (minIndex > maxIndex)
        {
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
    TestResults actualResult = TestList_LinearSearch(values, searchValue);
    if (result.found != actualResult.found)
    {
        printf("VERIFICATION FAILURE!! (found %s vs %s) %s, %s\n", result.found ? "true" : "false", actualResult.found ? "true" : "false", list, test);
    }
    else if (result.found == true && result.index != actualResult.index && values[result.index] != values[actualResult.index])
    {
        printf("VERIFICATION FAILURE!! (index %zu vs %zu) %s, %s\n", result.index, actualResult.index, list, test);
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

    size_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    threads.resize(numThreads);

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

                    char fileName[256];
                    sprintf_s(fileName, "out/%s.csv", MakeFns[makeIndex].name);
                    FILE* file = nullptr;
                    fopen_s(&file, fileName, "w+b");

                    // make a row for the sample counts
                    fprintf(file, "\"Sample Count\"");
                    for (size_t numValues = 1; numValues <= c_maxNumValues; ++numValues)
                        fprintf(file, ",\"%zu\"", numValues);
                    fprintf(file, "\n");

                    // for each test
                    std::vector<size_t> values;
                    for (size_t testIndex = 0; testIndex < countof(TestFns); ++testIndex)
                    {
                        std::vector<size_t> guessesMin(c_maxNumValues, ~size_t(0));
                        std::vector<size_t> guessesMax(c_maxNumValues, 0);
                        std::vector<float> guessesAvg(c_maxNumValues, 0.0f);
                        std::vector<size_t> guessesSingle(c_maxNumValues, 0);

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

                            guessesMin[numValues - 1] = guessMin;
                            guessesMax[numValues - 1] = guessMax;
                            guessesAvg[numValues - 1] = guessAverage;
                            guessesSingle[numValues - 1] = guessSingle;
                        }

                        fprintf(file, "\n\"%s Min\"", TestFns[testIndex].name);
                        for (size_t v : guessesMin)
                            fprintf(file, ",\"%zu\"", v);
                        fprintf(file, "\n\"%s Max\"", TestFns[testIndex].name);
                        for (size_t v : guessesMax)
                            fprintf(file, ",\"%zu\"", v);
                        fprintf(file, "\n\"%s Avg\"", TestFns[testIndex].name);
                        for (float v : guessesAvg)
                            fprintf(file, ",\"%f\"", v);
                        fprintf(file, "\n\"%s Single\"", TestFns[testIndex].name);
                        for (size_t v : guessesSingle)
                            fprintf(file, ",\"%zu\"", v);
                        fprintf(file, "\n");
                    }

                    // make a row for the sampling sequence itself
                    fprintf(file, "\n\"Sequence\"");
                    for (size_t v : values)
                        fprintf(file, ",\"%zu\"", v);
                    fprintf(file, "\n");

                    fclose(file);

                    printf("Done with %s\n", MakeFns[makeIndex].name);

                    makeIndex = nextRow.fetch_add(1);
                }
            }
        );
    }

    for (std::thread& t : threads)
        t.join();

    return 0;
}


/*

? should i transpose csv to make it easier to graph data?

 Analysis:
* Cubic: line fit does better often, but has large spikes, which is no good.
* Linear Outlier: Line fit does very bad.  The reason why, is there is a huge number at the end, which makes all the guesses be small indices. those are less then so it creeps up to the right value one at a time.
 * Possible way to help this worst case: randomly (or every other iteration) do a binary search step.
* Linear: line fit does very well compared to binary search.
* Log: Binary search does much better than line fit. Similar idea to linear outlier. It doesn't make enough progress
* Random: line fit does well compared to binary search.


Notes:
* TestList_LineFit() - the first 2 samples could reasonably be done in advance. Knowing min / max in the list isn't unreasonable.  It still beats binary search if you count those, but just by not as much.

* Hybrid: there's likely a sweet spot for when to do a binary step.  Maybe it's a tuneable constant, or maybe you do a binary step if you aren't making enough progress? not sure.

* Not testing perf. the code was written to be understandable, not for speed.

* mention online least squares fitting as a possibility? but it has a matrix inverse...
 * also, the "local fit" seems more appropriate
 * incremental least squares: https://blog.demofox.org/2016/12/22/incremental-least-squares-curve-fitting/

? why does it get better when the number of items in the list is larger?
 * i think it's because the numbers get denser. Try upping the max value for 1 run to verify / show this on the post.

Quadratic, cubic and beyond!
! describe quadratic and cubic algorithms
* Need to be able to take an y=f(x) function and invert it to be x=f(y) function.
* higher order is more complex.
* Function needs to be monotonic to be able to invert (unique x for each y).  Quadratic and cubic aren't always monotic, even when passing through a monotonic data set!!
 * thread: https://twitter.com/Atrix256/status/1108031089493184512
 * could possibly do a fit of a MONOTONIC polynomial that doesn't pass through the points. clamp to keep it in bounds.
 * possibly useful: https://math.stackexchange.com/questions/3129051/how-to-restrict-coefficients-of-polynomial-so-the-function-is-strictly-monotoni
 * and: https://en.wikipedia.org/wiki/Monotone_cubic_interpolation

*/