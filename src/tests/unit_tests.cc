#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "../config/app_config.h"
#include "../infrastructure/person_repository.h"

using config::isDateValid;
using infrastructure::from_pg_array;
using infrastructure::to_pg_array;

TEST(IsDateValid, AcceptsValidDates) {
    EXPECT_TRUE(isDateValid("1990-01-01"));
    EXPECT_TRUE(isDateValid("2024-02-29"));
    EXPECT_TRUE(isDateValid("2000-02-29"));
    EXPECT_TRUE(isDateValid("1800-01-01"));
    EXPECT_TRUE(isDateValid("9999-12-31"));
    EXPECT_TRUE(isDateValid("2400-02-29"));
}

TEST(IsDateValid, RejectsMalformed) {
    EXPECT_FALSE(isDateValid(""));
    EXPECT_FALSE(isDateValid("not-a-date"));
    EXPECT_FALSE(isDateValid("1990/01/01"));
    EXPECT_FALSE(isDateValid("99-01-01"));
    EXPECT_FALSE(isDateValid("1990-1-1"));
    EXPECT_FALSE(isDateValid("10000-01-01"));
    EXPECT_FALSE(isDateValid("1990-01-0a"));
    EXPECT_FALSE(isDateValid(" 90-01-01"));
    EXPECT_FALSE(isDateValid("1990-0a-01"));
    EXPECT_FALSE(isDateValid("19a0-01-01"));
}

TEST(IsDateValid, RejectsOutOfRangeFields) {
    EXPECT_FALSE(isDateValid("1799-01-01"));
    EXPECT_FALSE(isDateValid("1990-13-01"));
    EXPECT_FALSE(isDateValid("1990-00-01"));
    EXPECT_FALSE(isDateValid("1990-01-32"));
    EXPECT_FALSE(isDateValid("1990-01-00"));
    EXPECT_FALSE(isDateValid("1990-04-31"));
}

TEST(IsDateValid, HandlesLeapYearRules) {
    EXPECT_FALSE(isDateValid("2023-02-29"));
    EXPECT_FALSE(isDateValid("1900-02-29"));
    EXPECT_TRUE(isDateValid("2000-02-29"));
    EXPECT_TRUE(isDateValid("2024-02-29"));
}

TEST(PgArray, EmptyVectorRendersEmptyLiteral) {
    EXPECT_EQ(to_pg_array({}), "{}");
}

TEST(PgArray, RoundTripPreservesItems) {
    const std::vector<std::string> in{"C++", "Node", "Postgres"};
    const auto literal = to_pg_array(in);
    EXPECT_EQ(literal, "{\"C++\",\"Node\",\"Postgres\"}");
    const auto out = from_pg_array(literal);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], "C++");
    EXPECT_EQ(out[1], "Node");
    EXPECT_EQ(out[2], "Postgres");
}

TEST(PgArray, ShortLiteralsParseAsEmpty) {
    EXPECT_TRUE(from_pg_array("{}").empty());
    EXPECT_TRUE(from_pg_array("").empty());
}
