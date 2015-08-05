/*  Embedis - Embedded Dictionary Server
 Copyright (C) 2015 PatternAgents, LLC

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"

TEST(PubSub, Basics) {
    std::vector<std::string> a;

    embedis_test_init();

    embedis_test("UNSUBSCRIBE");

    a = {"subscribe", "temperature", "1"};
    EXPECT_EMBEDIS_ARRAY("SUBSCRIBE temperature", a);

    a = {"subscribe", "humidity", "2"};
    EXPECT_EMBEDIS_ARRAY("SUBSCRIBE humidity", a);

    embedis_test_interface(99);

    EXPECT_EMBEDIS_ERROR("SUBSCRIBE temperature");

    EXPECT_EMBEDIS_STRING("PUBLISH temperature 98.6", "1");

    embedis_test_interface(0);

    a = {"publish", "temperature", "98.6"};
    EXPECT_EMBEDIS_ARRAY("", a);

    a = {"unsubscribe", "temperature", "1"};
    EXPECT_EMBEDIS_ARRAY("UNSUBSCRIBE temperature", a);

    embedis_test_interface(99);

    EXPECT_EMBEDIS_STRING("PUBLISH temperature 101.5", "0");

    embedis_test_interface(0);

    EXPECT_EQ(embedis_test(""), "");


}