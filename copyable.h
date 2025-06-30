#pragma once

class copyable {
 protected:
  // 构造函数，默认为拷贝构造函数
  copyable() = default;
  // 析构函数，默认实现
  ~copyable() = default;

 public:
  // 定义拷贝构造函数，使用默认实现
  copyable(const copyable&) = default;
  // 重载赋值运算符，使用默认实现
  copyable& operator=(const copyable&) = default;
};