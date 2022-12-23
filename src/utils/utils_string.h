#ifndef UTILS_STRING_HPP_
#define UTILS_STRING_HPP_

#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace utils {
using shared_string_ptr = std::shared_ptr<char>;

class Padding {
 public:
	Padding(const std::string &string, std::size_t minimal_length, const char padding = ' ') :
		m_length(string.size() > minimal_length ? 0 : minimal_length - string.size()),
		m_padding(padding) {
	}

	const auto length() const { return m_length; }
	std::ostream &output(std::ostream &os) const;

 protected:
	std::ostream &pad(std::ostream &os, const std::size_t length) const;

 private:
	std::size_t m_length;
	char m_padding;
};

inline std::ostream &operator<<(std::ostream &os, const Padding &padding) { return padding.output(os); }

class SpacedPadding : public Padding {
 public:
	SpacedPadding(const std::string &string, std::size_t minimal_length, const char padding = ' ') :
		Padding(string, minimal_length, padding) {
	}

	std::ostream &output(std::ostream &os) const;
};

inline std::ostream &operator<<(std::ostream &os, const SpacedPadding &padding) { return padding.output(os); }

class AbstractStringWriter {
 public:
	using shared_ptr = std::shared_ptr<AbstractStringWriter>;

	virtual ~AbstractStringWriter() = default;
	[[nodiscard]] virtual const char *get_string() const = 0;
	virtual void set_string(const char *data) = 0;
	virtual void append_string(const char *data) = 0;
	[[nodiscard]] virtual size_t length() const = 0;
	virtual void clear() = 0;
};

class DelegatedStringWriter : public AbstractStringWriter {
 public:
	DelegatedStringWriter(char *&managed) : m_delegated_string_(managed) {}
	virtual const char *get_string() const override { return m_delegated_string_; }
	virtual void set_string(const char *string) override;
	virtual void append_string(const char *string) override;
	virtual size_t length() const override { return m_delegated_string_ ? strlen(m_delegated_string_) : 0; }
	virtual void clear() override;

 private:
	char *&m_delegated_string_;
};

class AbstractStdStringWriter : public AbstractStringWriter {
 public:
	virtual const char *get_string() const override { return string().c_str(); }
	virtual void set_string(const char *string) override { this->string() = string; }
	virtual void append_string(const char *string) override { this->string() += string; }
	virtual size_t length() const override { return string().length(); }
	virtual void clear() override { string().clear(); }

 private:
	virtual std::string &string() = 0;
	virtual const std::string &string() const = 0;
};

class StdStringWriter : public AbstractStdStringWriter {
 private:
	virtual std::string &string() override { return m_string_; }
	virtual const std::string &string() const override { return m_string_; }

	std::string m_string_;
};

class DelegatedStdStringWriter : public AbstractStringWriter {
 public:
	DelegatedStdStringWriter(std::string &string) : m_string_(string) {}
	virtual const char *get_string() const override { return m_string_.c_str(); }
	virtual void set_string(const char *string) override { m_string_ = string; }
	virtual void append_string(const char *string) override { m_string_ += string; }
	virtual size_t length() const override { return m_string_.length(); }
	virtual void clear() override { m_string_.clear(); }

 private:
	std::string &m_string_;
};

std::string RemoveColors(char *string);
std::string RemoveColors(std::string string);
shared_string_ptr GetStringWithoutColors(const char *string);
std::string GetStringWithoutColors(const std::string &string);

bool IsAbbr(const char *arg1, const char *arg2);
inline int IsAbbrev(const std::string &arg1, const char *arg2) { return IsAbbr(arg1.c_str(), arg2); }


// строку в нижний регистр
void ConvertToLow(std::string &text);
void ConvertToLow(char *text);

/**
 * Разделить строку на элементы по разделителю и записать в вектор tokens.
 * @param tokens - приемный вектор.
 * @param s - разделяемая строка.
 * @param delimiter - символ-разделитель, по умолчаниб - пробел.
 */
void Split(std::vector<std::string> &tokens, const std::string& s, char delimiter = ' ');

/**
 * Обрезать пробелы слева.
 */
void TrimLeft(std::string &s);

/**
 *  Обрезать пробелы справа.
 */
void TrimRight(std::string &s)

/**
 * Обрезать пробелы справа и слева.
 */;
void Trim(std::string &s);

/**
 * Обрезать пробелы слева, вернуть копию.
 */
std::string TrimLeftCopy(std::string s);

/**
 *  Обрезать пробелы справа, вернуть копию.
 */

std::string TrimRightCopy(std::string s);

/**
 * Обрезать пробелы справа и слева, вернуть копию.
 */;
std::string TrimCopy(std::string s);

// сконвертировать в WIN для сортировки
void ConvertKtoW(std::string &text);

// обратно
void ConvertWtoK(std::string &text);

// На копии
std::string SubstKtoW(std::string s);

// На копии
std::string SubstWtoK(std::string s);

// соортировка KOI8-R строк, а-я
void SortKoiString(std::vector<std::string> &str);

// соортировка KOI8-R строк, я-а
void SortKoiStringReverse(std::vector<std::string> &str);

// Замена . и _ на пробел
std::string FixDot(std::string s);

// строку в нижний регистр
std::string SubstStrToLow(std::string s);

// строку в верхний регистр
std::string SubstStrToUpper(std::string s);

}
#endif // UTILS_STRING_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
