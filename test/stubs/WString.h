/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Native-only shim for the Arduino `String` class, backed by std::string.
 * Implements the subset of the Arduino String API used by the decoder path
 * (AmsData, IEC6205621/DSMR text parsing, hexutils, LNG/LNG2). Only compiled
 * for native unit tests — on device the real Arduino String is used.
 */
#ifndef _NATIVE_WSTRING_H
#define _NATIVE_WSTRING_H

#include <string>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cctype>

#ifndef HEX
#define HEX 16
#endif
#ifndef DEC
#define DEC 10
#endif

class String {
    std::string s;
    static std::string num(long v, int base) {
        if (base == HEX) { char b[32]; snprintf(b, sizeof(b), "%lX", (unsigned long)v); return b; }
        char b[32]; snprintf(b, sizeof(b), "%ld", v); return b;
    }
public:
    String() {}
    String(const char* p) : s(p ? p : "") {}
    String(const std::string& o) : s(o) {}
    String(const String& o) : s(o.s) {}
    explicit String(char c) { s = std::string(1, c); }
    String(unsigned char v, int base = DEC) : s(num(v, base)) {}
    String(int v, int base = DEC)            : s(num(v, base)) {}
    String(unsigned int v, int base = DEC)   : s(num((long)v, base)) {}
    String(long v, int base = DEC)           : s(num(v, base)) {}
    String(unsigned long v, int base = DEC)  : s(num((long)v, base)) {}
    String(double v, int decimals = 2) { char b[64]; snprintf(b, sizeof(b), "%.*f", decimals, v); s = b; }

    String& operator=(const char* p) { s = p ? p : ""; return *this; }
    String& operator=(const String& o) { s = o.s; return *this; }

    String& operator+=(const char* p) { if (p) s += p; return *this; }
    String& operator+=(const String& o) { s += o.s; return *this; }
    String& operator+=(char c) { s += c; return *this; }

    friend String operator+(const String& a, const String& b) { String r(a); r.s += b.s; return r; }
    friend String operator+(const String& a, const char* b) { String r(a); if (b) r.s += b; return r; }
    friend String operator+(const char* a, const String& b) { String r(a); r.s += b.s; return r; }

    bool operator==(const char* p) const { return s == (p ? p : ""); }
    bool operator==(const String& o) const { return s == o.s; }
    bool operator!=(const char* p) const { return !(*this == p); }
    bool operator!=(const String& o) const { return s != o.s; }
    bool equals(const char* p) const { return *this == p; }
    bool equals(const String& o) const { return s == o.s; }

    friend bool operator==(const char* a, const String& b) { return b == a; }

    unsigned int length() const { return (unsigned int)s.length(); }
    const char* c_str() const { return s.c_str(); }
    char charAt(unsigned int i) const { return i < s.length() ? s[i] : 0; }
    char operator[](unsigned int i) const { return charAt(i); }

    // Arduino substring(from[, to]) — `to` is exclusive
    String substring(unsigned int from) const {
        if (from >= s.length()) return String();
        return String(s.substr(from));
    }
    String substring(unsigned int from, unsigned int to) const {
        if (from >= s.length() || to <= from) return String();
        if (to > s.length()) to = (unsigned int)s.length();
        return String(s.substr(from, to - from));
    }

    int indexOf(char c, unsigned int from = 0) const {
        size_t r = s.find(c, from); return r == std::string::npos ? -1 : (int)r;
    }
    int indexOf(const char* p, unsigned int from = 0) const {
        size_t r = s.find(p, from); return r == std::string::npos ? -1 : (int)r;
    }
    int indexOf(const String& o, unsigned int from = 0) const { return indexOf(o.c_str(), from); }
    int lastIndexOf(char c) const {
        size_t r = s.rfind(c); return r == std::string::npos ? -1 : (int)r;
    }

    bool startsWith(const char* p) const { return p && s.rfind(p, 0) == 0; }
    bool startsWith(const String& o) const { return startsWith(o.c_str()); }
    bool endsWith(const char* p) const {
        std::string q = p ? p : ""; return s.length() >= q.length() &&
            s.compare(s.length() - q.length(), q.length(), q) == 0;
    }

    void trim() {
        size_t b = 0, e = s.length();
        while (b < e && isspace((unsigned char)s[b])) b++;
        while (e > b && isspace((unsigned char)s[e - 1])) e--;
        s = s.substr(b, e - b);
    }
    void toUpperCase() { for (auto& c : s) c = (char)toupper((unsigned char)c); }
    void toLowerCase() { for (auto& c : s) c = (char)tolower((unsigned char)c); }

    long toInt() const { return strtol(s.c_str(), nullptr, 10); }
    double toDouble() const { return strtod(s.c_str(), nullptr); }
    float toFloat() const { return (float)strtod(s.c_str(), nullptr); }
    bool isEmpty() const { return s.empty(); }
};

#endif
