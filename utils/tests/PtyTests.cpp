#include <gtest/gtest.h>

#include <os/utils/pty.hpp>
#include <stdio.h>
#include <iostream>
#include <unistd.h>

TEST(PtyCommunication, CreateDelete) {
    auto ptys = os::pty("local1", "local2");
    FILE* l1 = fopen("local1", "r+");
    ASSERT_FALSE(l1 == NULL);
    FILE* l2 = fopen("local2", "r+");
    ASSERT_FALSE(l2 == NULL);
    fclose(l1);
    fclose(l2);

    ptys.close();

    l1 = fopen("local1", "r");
    EXPECT_TRUE(l1 == NULL);
    l2 = fopen("local2", "r");
    EXPECT_TRUE(l2 == NULL);
}


TEST(PtyCommunication, Communicate) {
    char buffer[10];
    auto ptys = os::pty("local1", "local2");
    FILE* l1 = fopen("local1", "r+");
    ASSERT_FALSE(l1 == NULL);
    FILE* l2 = fopen("local2", "r+");
    ASSERT_FALSE(l2 == NULL);

    fwrite("a", sizeof(char), 1, l1);
    fflush(l1);
    fread(buffer, sizeof(char), 1, l2);
    EXPECT_EQ('a', buffer[0]);
    fwrite("b", sizeof(char), 1, l2);
    fflush(l2);
    fread(buffer, sizeof(char), 1, l1);
    EXPECT_EQ('b', buffer[0]);

    fclose(l1);
    fclose(l2);
}
