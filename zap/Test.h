//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// This header file is for allowing test access to classes without
// Having to include the test framework headers
#ifndef _TEST_H_
#define _TEST_H_

// Taken from gtest
#define FRIEND_TEST(test_case_name, test_name)\
friend class test_case_name##_##test_name##_Test


#endif // _TEST_H_
