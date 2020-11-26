#include <assert.h>
#include <stdarg.h>  // va_list, etc.
#include <stdio.h>   // vprintf

#include "gc_heap.h"
#include "greatest.h"
#include "my_runtime.h"

using gc_heap::Alloc;
using gc_heap::gHeap;

GLOBAL_STR(kString1, "food");
GLOBAL_STR(kWithNull, "foo\0bar");

TEST print_test() {
  print(kString1);
  println_stderr(kString1);

  print(kWithNull);
  println_stderr(kWithNull);

  PASS();
}

TEST str_to_int_test() {
  int i;
  bool ok;

  ok = _str_to_int(NewStr("345"), &i, 10);
  ASSERT(ok);
  ASSERT_EQ_FMT(345, i, "%d");

  // Hack to test slicing.  Truncated "345" at "34".
  ok = _str_to_int(NewStr("345", 2), &i, 10);
  ASSERT(ok);
  ASSERT_EQ_FMT(34, i, "%d");

  ok = _str_to_int(NewStr("1234567890"), &i, 10);
  ASSERT(ok);
  ASSERT(i == 1234567890);

  // overflow
  ok = _str_to_int(NewStr("12345678901234567890"), &i, 10);
  ASSERT(!ok);

  // underflow
  ok = _str_to_int(NewStr("-12345678901234567890"), &i, 10);
  ASSERT(!ok);

  // negative
  ok = _str_to_int(NewStr("-123"), &i, 10);
  ASSERT(ok);
  ASSERT(i == -123);

  // Leading space is OK!
  ok = _str_to_int(NewStr(" -123"), &i, 10);
  ASSERT(ok);
  ASSERT(i == -123);

  // Trailing space is OK!  NOTE: This fails!
  ok = _str_to_int(NewStr(" -123  "), &i, 10);
  ASSERT(ok);
  ASSERT(i == -123);

  // Empty string isn't an integer
  ok = _str_to_int(NewStr(""), &i, 10);
  ASSERT(!ok);

  ok = _str_to_int(NewStr("xx"), &i, 10);
  ASSERT(!ok);

  // Trailing garbage
  ok = _str_to_int(NewStr("42a"), &i, 10);
  ASSERT(!ok);

  i = to_int(NewStr("ff"), 16);
  ASSERT(i == 255);

  // strtol allows 0x prefix
  i = to_int(NewStr("0xff"), 16);
  ASSERT(i == 255);

  // TODO: test ValueError here
  // i = to_int(NewStr("0xz"), 16);

  i = to_int(NewStr("0"), 16);
  ASSERT(i == 0);

  i = to_int(NewStr("077"), 8);
  ASSERT_EQ_FMT(63, i, "%d");

  bool caught = false;
  try {
    i = to_int(NewStr("zzz"));
  } catch (ValueError* e) {
    caught = true;
  }
  ASSERT(caught);

  PASS();
}

TEST str_replace_test() {
  Str* s = kString1->replace(NewStr("o"), NewStr("12"));
  ASSERT(str_equals(NewStr("f1212d"), s));
  print(s);

  // BUG in corner case!
  Str* s2 = NewStr("foo")->replace(NewStr("o"), NewStr("123"));
  ASSERT(str_equals(NewStr("f123123"), s2));
  print(s2);

  Str* s3 = NewStr("foxo")->replace(NewStr("o"), NewStr("123"));
  ASSERT(str_equals(NewStr("f123x123"), s3));
  print(s3);

  Str* s4 = kWithNull->replace(NewStr("a"), NewStr("XX"));
  print(s4);
  // Explicit length because of \0
  ASSERT(str_equals(NewStr("foo\0bXXr", 8), s4));

  PASS();
}

TEST str_funcs_test() {
  log("slice()");
  ASSERT(str_equals(NewStr("f"), kString1->index(0)));

  ASSERT(str_equals(NewStr("d"), kString1->index(-1)));

  ASSERT(str_equals(NewStr("ood"), kString1->slice(1)));
  ASSERT(str_equals(NewStr("oo"), kString1->slice(1, 3)));
  ASSERT(str_equals(NewStr("oo"), kString1->slice(1, -1)));
  ASSERT(str_equals(NewStr("o"), kString1->slice(-3, -2)));
  ASSERT(str_equals(NewStr("fo"), kString1->slice(-4, -2)));

  ASSERT(str_equals(NewStr("foodfood"), str_concat(kString1, kString1)));

  log("str_repeat()");
  Str* s = NewStr("abc");

  // -1 is allowed by Python and used by Oil!
  ASSERT(str_equals(kEmptyString, str_repeat(s, -1)));
  ASSERT(str_equals(kEmptyString, str_repeat(s, 0)));

  Str* r1 = str_repeat(s, 1);
  ASSERT(str_equals(s, r1));

  Str* r3 = str_repeat(s, 3);
  ASSERT(str_equals(NewStr("abcabcabc"), r3));

  log("join()");
  auto foo = NewStr("foo");
  auto bar = NewStr("bar");

  auto L1 = Alloc<List<Str*>>(std::initializer_list<Str*>{foo, bar});
  ASSERT(str_equals(NewStr("foobar"), kEmptyString->join(L1)));

  // Join by NUL
  ASSERT(str_equals(NewStr("foo\0bar", 7), NewStr("\0", 1)->join(L1)));

  auto L2 = Alloc<List<Str*>>(std::initializer_list<Str*>{foo});
  ASSERT(str_equals(NewStr("foo"), kEmptyString->join(L2)));

  auto empty_list = Alloc<List<Str*>>(std::initializer_list<Str*>{});

  auto empty = kEmptyString->join(empty_list);
  ASSERT(str_equals(kEmptyString, empty));
  ASSERT_EQ(0, len(empty));

  auto j1 = (NewStr(" "))->join(empty_list);
  ASSERT(str_equals(kEmptyString, j1));
  ASSERT_EQ(0, len(j1));

  PASS();
}

TEST str_iters_test() {
  for (StrIter it(kString1); !it.Done(); it.Next()) {
    print(it.Value());
  }

  PASS();
}

void ListFunc(std::initializer_list<Str*> init) {
  log("init.size() = %d", init.size());
}

TEST list_funcs_test() {
  auto ints = Alloc<List<int>>();
  ints->extend(std::initializer_list<int>{5, 6, 7, 8});

  List<int>* slice1 = ints->slice(1);
  ASSERT_EQ(3, len(slice1));
  ASSERT_EQ(6, slice1->index(0));

  List<int>* slice2 = ints->slice(-4, -2);
  ASSERT_EQ(2, len(slice2));
  ASSERT_EQ(5, slice2->index(0));

  log("-- before pop(0)");
  for (int i = 0; i < len(ints); ++i) {
    log("ints[%d] = %d", i, ints->index(i));
  }
  ASSERT_EQ(4, len(ints));  // [5, 6, 7, 8]

  log("pop()");

  ints->pop();  // [5, 6, 7]
  ASSERT_EQ(3, len(ints));
  ASSERT_EQ_FMT(5, ints->index(0), "%d");
  ASSERT_EQ_FMT(6, ints->index(1), "%d");
  ASSERT_EQ_FMT(7, ints->index(2), "%d");

  log("pop(0)");

  ints->pop(0);  // [6, 7]
  ASSERT_EQ(2, len(ints));
  ASSERT_EQ_FMT(6, ints->index(0), "%d");
  ASSERT_EQ_FMT(7, ints->index(1), "%d");

  ints->reverse();
  ASSERT_EQ(2, len(ints));  // [7, 6]

  ASSERT_EQ_FMT(7, ints->index(0), "%d");
  ASSERT_EQ_FMT(6, ints->index(1), "%d");

  ints->append(9);  // [7, 6, 9]
  ASSERT_EQ(3, len(ints));

  ints->reverse();  // [9, 6, 7]
  ASSERT_EQ(9, ints->index(0));
  ASSERT_EQ(6, ints->index(1));
  ASSERT_EQ(7, ints->index(2));

  ints->clear();
  ASSERT_EQ(0, len(ints));
  ASSERT_EQ(0, ints->slab_->items_[0]);  // make sure it's zero'd

  auto L = list_repeat<Str*>(nullptr, 3);
  ASSERT_EQ(3, len(L));

  auto L2 = list_repeat<bool>(true, 3);
  log("list_repeat length = %d", len(L2));
  log("item 0 %d", L2->index(0));
  log("item 1 %d", L2->index(1));

  // Not implemented since we don't use it in Oil.
  // ints->sort();

  PASS();
}

TEST list_iters_test() {
  log("  forward iteration over list");
  auto ints = Alloc<List<int>>();
  ints->extend(std::initializer_list<int>{1, 2, 3});

  for (ListIter<int> it(ints); !it.Done(); it.Next()) {
    int x = it.Value();
    log("x = %d", x);
  }

  log("  backward iteration over list");
  for (ReverseListIter<int> it(ints); !it.Done(); it.Next()) {
    int x = it.Value();
    log("x = %d", x);
  }

  // hm std::initializer_list is "first class"
  auto strs = {NewStr("foo"), NewStr("bar")};
  ListFunc(strs);

  PASS();
}

TEST sort_test() {
  ASSERT_EQ(0, int_cmp(0, 0));
  ASSERT_EQ(-1, int_cmp(0, 5));
  ASSERT_EQ(1, int_cmp(0, -5));

  auto a = NewStr("a");
  auto aa = NewStr("aa");
  auto b = NewStr("b");

  ASSERT_EQ(0, str_cmp(kEmptyString, kEmptyString));
  ASSERT_EQ(-1, str_cmp(kEmptyString, a));
  ASSERT_EQ(-1, str_cmp(a, aa));
  ASSERT_EQ(-1, str_cmp(a, b));

  ASSERT_EQ(1, str_cmp(b, a));
  ASSERT_EQ(1, str_cmp(b, kEmptyString));

  auto strs = Alloc<List<Str*>>();
  strs->append(NewStr("c"));
  strs->append(NewStr("a"));
  strs->append(NewStr("b"));
  strs->append(kEmptyString);
  ASSERT_EQ(4, len(strs));  // ['c', 'a', 'b', '']

#if 0
  strs->sort();  // ['a', 'b', 'c']
  ASSERT_EQ(4, len(strs));
  ASSERT(str_equals(kEmptyString, strs->index(0)));
  ASSERT(str_equals(NewStr("a"), strs->index(1)));
  ASSERT(str_equals(NewStr("b"), strs->index(2)));
  ASSERT(str_equals(NewStr("c"), strs->index(3)));
#endif

  PASS();
}

TEST contains_test() {
  bool b;

  // TODO: Implement substring
#if 0
  b = str_contains(NewStr("foo"), NewStr("oo"));
  ASSERT(b == true);

  b = str_contains(NewStr("foo"), NewStr("ood"));
  ASSERT(b == false);
#endif

  b = str_contains(NewStr("foo\0a", 5), NewStr("a"));
  ASSERT(b == true);

  // this ends with a NUL, but also has a NUL terinator.
  Str* s = NewStr("foo\0", 4);
  b = str_contains(s, NewStr("\0", 1));
  ASSERT(b == true);

  log("  List<Str*>");
  auto strs = Alloc<List<Str*>>();
  strs->append(NewStr("bar"));

  b = list_contains(strs, NewStr("foo"));
  ASSERT(b == false);

  strs->append(NewStr("foo"));
  b = list_contains(strs, NewStr("foo"));
  ASSERT(b == true);

  log("  ints");
  auto ints = Alloc<List<int>>(std::initializer_list<int>{1, 2, 3});
  b = list_contains(ints, 1);
  ASSERT(b == true);

  b = list_contains(ints, 42);
  ASSERT(b == false);

  log("  floats");
  auto floats =
      Alloc<List<double>>(std::initializer_list<double>{0.5, 0.25, 0.0});
  b = list_contains(floats, 0.0);
  log("b = %d", b);
  b = list_contains(floats, 42.0);
  log("b = %d", b);

  PASS();
}

TEST formatter_test() {
  gBuf.reset();
  gBuf.write_const("[", 1);
  gBuf.format_s(NewStr("bar"));
  gBuf.write_const("]", 1);
  log("value = %s", gBuf.getvalue()->data_);

  gBuf.format_d(42);
  gBuf.write_const("-", 1);
  gBuf.format_d(42);
  gBuf.write_const(".", 1);
  log("value = %s", gBuf.getvalue()->data_);

  PASS();
}

GLOBAL_STR(b, "b");
GLOBAL_STR(bb, "bx");

TEST collect_test() {
  gHeap.Init(1 << 8);  // 1 KiB

  auto s = NewStr("abcdefg");
  int total = 0;
  for (int i = 0; i < 40; ++i) {
    s = s->replace(b, bb);
    total += len(s);

    // hit NUL termination path
    s = NewStr("NUL");
    total += len(s);

    // log("i = %d", i);
    // log("len(s) = %d", len(s));
  }
  log("total = %d", total);

  PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char** argv) {
  gHeap.Init(1 << 20);

  GREATEST_MAIN_BEGIN();

  RUN_TEST(print_test);
  RUN_TEST(str_to_int_test);
  RUN_TEST(str_replace_test);
  RUN_TEST(str_funcs_test);
  RUN_TEST(str_iters_test);
  RUN_TEST(list_funcs_test);
  RUN_TEST(list_iters_test);
  RUN_TEST(sort_test);
  RUN_TEST(contains_test);

  RUN_TEST(formatter_test);
  RUN_TEST(collect_test);

  GREATEST_MAIN_END(); /* display results */
  return 0;
}
