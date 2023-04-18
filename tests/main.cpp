//
// Created by matvey on 18.04.23.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(test_case_name, test_name)
{
    ASSERT_EQ(1, 1) << "1 is not equal 0";
}

int main(int argc, char **argv){
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
