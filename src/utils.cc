#include <drogon/drogon.h>

using namespace std;

void logInfo(const string message)
{
    cout << message << endl;
}

void logException(const char *message)
{
    cerr << "error:" << message << endl;
}

const int MAX_VALID_YR = 9999;
const int MIN_VALID_YR = 1800;

bool isLeap(int year)
{
    return bool(((year % 4 == 0) &&
                 (year % 100 != 0)) ||
                (year % 400 == 0));
}

bool isDateValid(string date)
{
    if (date.size() != 10) {
        return false;
    }

    vector<int> partsOfDate;

    stringstream dateStream(date);

    string split;
    while (getline(dateStream, split, '-'))
    {
        try {
            partsOfDate.push_back(stoi(split));
        } catch(exception &e) {
            break;
        }
    }

    if (partsOfDate.size() != 3) {
        return false;
    }

    auto day = partsOfDate.at(2);
    auto month = partsOfDate.at(1);
    auto year = partsOfDate.at(0);

    if (year > MAX_VALID_YR ||
        year < MIN_VALID_YR)
        return false;
    if (month < 1 || month > 12)
        return false;
    if (day < 1 || day > 31)
        return false;

    if (month == 2)
    {
        if (isLeap(year))
            return (day <= 29);
        else
            return (day <= 28);
    }

    if (month == 4 || month == 6 ||
        month == 9 || month == 11)
        return (day <= 30);

    return true;
}
