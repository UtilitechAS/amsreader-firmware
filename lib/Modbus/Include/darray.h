
/*
    Very Basic Dynamic Array

    Copyright (C) 2020 Alexander Emelianov (a.m.emelianov@gmail.com)
	https://github.com/emelianov/modbus-esp8266
	This code is licensed under the BSD New License. See LICENSE.txt for more info.
*/
template <typename T, int SIZE, int INCREMENT>
class DArray {
  public:
  typedef bool (*Compare)(T);
  T* data = nullptr;
  size_t resSize = 0;
  size_t last = 0;
  bool isEmpty = true;
  DArray(size_t i = SIZE) {
    data = (T*)malloc(i * sizeof(T));
    if (data) resSize = i;
  }
  size_t push_back(const T& v) {
    if (!data) {
      data = (T*)malloc(resSize * sizeof(T));
      if (!data) return 1;
    }
    if (last >= resSize - 1) {
      if (INCREMENT == 0) return last + 1;
      void* tmp = realloc(data, (resSize + INCREMENT) * sizeof(T));
      if (!tmp) return last + 1;
      resSize += INCREMENT;
      data = (T*)tmp;
    }
    if (!isEmpty)
      last++;
    else
      isEmpty = false;
    data[last] = v;
    return last;
  }
  size_t size() {
    if (isEmpty) return 0;
    return last + 1;
  }
  template <class UnaryPredicate>
  size_t find(UnaryPredicate func, size_t i = 0) {
    if (isEmpty) return 1;
    for (; i <= last; i++)
      if (func(data[i])) break;
    return i;
  }

  void remove(size_t i) {
    if (isEmpty) return;
    if (i > last) return;
    if (last == 0) {
      isEmpty = true;
      return;
    }
    if (i < last)
      memcpy(&data[i], &data[i + 1], (last - i) * sizeof(T));
    last --;
  }
  T operator[](int i) {
    return data[i];
  }
  T* entry(size_t i) {
    if (i > last) return nullptr;
      return &data[i];
  }
};