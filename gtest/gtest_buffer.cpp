// Copyright (c) 2012 Plenluno All rights reserved.

#include <gtest/gtest.h>
#include <libnode/buffer.h>

namespace libj {
namespace node {

TEST(GTestBuffer, TestCreate) {
    Buffer::Ptr buf = Buffer::create();
    ASSERT_TRUE(buf);
}

TEST(GTestBuffer, TestCreate2) {
    Buffer::Ptr buf = Buffer::create("abc", 3);
    ASSERT_TRUE(buf);
    ASSERT_EQ(3, buf->length());
    ASSERT_TRUE(buf->toString()->equals(String::create("abc")));
}

TEST(GTestBuffer, TestCreate3) {
    JsTypedArray<UByte>::Ptr ary = JsTypedArray<UByte>::create();
    ary->add(static_cast<UByte>('0'));
    ary->add(static_cast<UByte>('1'));
    Buffer::Ptr buf = Buffer::create(ary);
    ASSERT_TRUE(buf);
    ASSERT_EQ(2, buf->length());
    ASSERT_TRUE(buf->toString()->equals(String::create("01")));
}

TEST(GTestBuffer, TestCreate4) {
    const UByte d[] = {
        0xe3, 0x81, 0x82,
        0xe3, 0x81, 0x84,
        0xe3, 0x81, 0x86,
        0x00
    };
    String::CPtr str = String::create(d, String::UTF8);
    ASSERT_EQ(3, str->length());
    Buffer::Ptr buf = Buffer::create(str, Buffer::UTF8);
    ASSERT_TRUE(buf);
    ASSERT_EQ(9, buf->length());
    ASSERT_EQ(0x86, buf->get(8));
    ASSERT_TRUE(buf->toString()->equals(str));
}

TEST(GTestBuffer, TestByteLength) {
    const UByte d[] = {
        0xe3, 0x81, 0x82,
        0xe3, 0x81, 0x84,
        0xe3, 0x81, 0x86,
        0x00
    };
    String::CPtr str = String::create(d, String::UTF8);
    ASSERT_EQ(9, Buffer::byteLength(str));
}

TEST(GTestBuffer, TestCopy) {
    Buffer::Ptr buf1 = Buffer::create("abcde", 5);
    Buffer::Ptr buf2 = Buffer::create("xyz", 3);
    Buffer::Ptr buf = Buffer::create(5);
    ASSERT_EQ(3, buf1->copy(buf, 0, 1, 4));
    ASSERT_EQ(2, buf2->copy(buf, 3));
    ASSERT_EQ(5, buf->length());
    String::CPtr str = buf->toString();
    ASSERT_TRUE(str->equals(String::create("bcdxy")));
}

TEST(GTestBuffer, TestConcat) {
    Buffer::Ptr buf1 = Buffer::create("abc", 3);
    Buffer::Ptr buf2 = Buffer::null();
    Buffer::Ptr buf3 = Buffer::create("xyz", 3);
    JsArray::Ptr ary = JsArray::create();
    ary->add(buf1);
    ary->add(buf2);
    ary->add(buf3);
    Buffer::Ptr buf = Buffer::concat(ary, 5);
    ASSERT_EQ(5, buf->length());
    String::CPtr str = buf->toString();
    ASSERT_TRUE(str->equals(String::create("abcxy")));
}

TEST(GTestBuffer, TestConcat2) {
    Buffer::Ptr buf1 = Buffer::create("abc", 3);
    Buffer::Ptr buf2 = Buffer::create("xyz", 3);
    Buffer::Ptr buf = buf1->concat(buf2);
    ASSERT_TRUE(buf->toString()->equals(String::create("abcxyz")));
}

TEST(GTestBuffer, TestWriteRead) {
    Buffer::Ptr buf = Buffer::create(2);
    Byte wb = 15;
    ASSERT_TRUE(buf->writeInt8(wb, 1));
    Byte rb = 0;
    ASSERT_TRUE(buf->readInt8(1, &rb));
    ASSERT_EQ(wb, rb);
}

TEST(GTestBuffer, TestLength) {
    Size len = 35472;
    Buffer::Ptr buf = Buffer::create(len);
    ASSERT_EQ(len, buf->length());
}

}  // namespace node
}  // namespace libj
