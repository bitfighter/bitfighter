#pragma once
#include <gtest/gtest.h>

class BitfighterTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override;
    void TearDown() override;
};
