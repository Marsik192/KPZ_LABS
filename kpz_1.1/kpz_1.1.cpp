#include <iostream>
#include <cwchar>
#include <clocale>
#include <map>
#include <cmath>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

// Визначення типів токенів
enum TokenTypes { DELIMITER, NUMBER, VARIABLE, UNKNOWN };

class ExpressionParser {
public:
    ExpressionParser();
    double evaluateExpression(wchar_t* expression);
    std::wstring differentiate(wchar_t* expression, const std::wstring& variable);
    std::wstring integrate(wchar_t* expression, const std::wstring& variable);

private:
    wchar_t* expressionPointer;  // вказівник на вираз
    wchar_t currentToken[80];    // поточний токен
    TokenTypes tokenType;         // тип токена
    std::map<std::wstring, double> variables; // Змінні

    void evaluateTerm(double& result);
    void evaluateFactor(double& result);
    void evaluatePower(double& result);
    void evaluateUnary(double& result);
    void evaluatePrimary(double& result);
    void parseToken();
    void syntaxError(int errorCode);
    bool isDelimiter(wchar_t c);
    double findVariable(const wchar_t* var);
    void assignVariable(const wchar_t* var, double value);

    std::wstring differentiateTerm();
    std::wstring differentiateFactor();
    std::wstring differentiatePower();
    std::wstring differentiateUnary();
    std::wstring differentiatePrimary();

    std::wstring integrateTerm();
    std::wstring integrateFactor();
    std::wstring integratePower();
    std::wstring integrateUnary();
    std::wstring integratePrimary();

    std::wstring differentiateVariable(const std::wstring& var, const std::wstring& diffVar);
    std::wstring integrateVariable(const std::wstring& var, const std::wstring& intVar);
};

ExpressionParser::ExpressionParser() {
    expressionPointer = nullptr;
}

double ExpressionParser::evaluateExpression(wchar_t* expression) {
    double result;
    expressionPointer = expression;

    parseToken();
    if (!*currentToken) {
        syntaxError(2);  // немає виразу
        return 0.0;
    }
    evaluateTerm(result);
    if (*currentToken)
        syntaxError(0);
    return result;
}

// Метод для обробки додавання і віднімання
void ExpressionParser::evaluateTerm(double& result) {
    wchar_t op;
    double temp;

    evaluateFactor(result);

    while ((op = *currentToken) == L'+' || op == L'-') {
        parseToken();
        evaluateFactor(temp);
        switch (op) {
        case L'-':
            result -= temp;
            break;
        case L'+':
            result += temp;
            break;
        }
    }
}

// Метод для обробки множення і ділення
void ExpressionParser::evaluateFactor(double& result) {
    wchar_t op;
    double temp;

    evaluatePower(result);
    while ((op = *currentToken) == L'*' || op == L'/' || op == L'%') {
        parseToken();
        evaluatePower(temp);
        switch (op) {
        case L'*':
            result *= temp;
            break;
        case L'/':
            result /= temp;
            break;
        case L'%':
            result = std::fmod(result, temp);
            break;
        }
    }
}

// Метод для обробки піднесення до степеня
void ExpressionParser::evaluatePower(double& result) {
    double temp, ex;

    evaluateUnary(result);
    if (*currentToken == L'^') {
        parseToken();
        evaluateUnary(temp);
        ex = result;
        if (temp == 0.0) {
            result = 1.0;
            return;
        }
        result = std::pow(ex, temp);
    }
}

// Метод для обробки унарних операторів
void ExpressionParser::evaluateUnary(double& result) {
    wchar_t op = 0;
    if ((tokenType == DELIMITER) && (*currentToken == L'+' || *currentToken == L'-')) {
        op = *currentToken;
        parseToken();
    }
    evaluatePrimary(result);
    if (op == L'-')
        result = -result;
}

// Метод для обробки дужок, чисел та змінних
void ExpressionParser::evaluatePrimary(double& result) {
    if (*currentToken == L'(') {
        parseToken();
        evaluateTerm(result);
        if (*currentToken != L')')
            syntaxError(1);
        parseToken();
    }
    else {
        switch (tokenType) {
        case NUMBER:
            result = wcstod(currentToken, nullptr);
            parseToken();
            return;
        case VARIABLE: {
            wchar_t tempToken[80];
            wcscpy_s(tempToken, currentToken); // Використовуємо безпечну версію wcscpy_s
            parseToken();
            if (*currentToken == L'=') {
                parseToken();
                evaluateTerm(result);
                assignVariable(tempToken, result);
            }
            else {
                result = findVariable(tempToken);
            }
            return;
        }
        default:
            syntaxError(0);
        }
    }
}

// Метод для розбору токенів
void ExpressionParser::parseToken() {
    wchar_t* temp = currentToken;

    tokenType = UNKNOWN;
    *temp = L'\0';

    if (!*expressionPointer)
        return;

    while (iswspace(*expressionPointer))
        ++expressionPointer;

    if (wcschr(L"+-*/%^=()", *expressionPointer)) {
        tokenType = DELIMITER;
        *temp++ = *expressionPointer++;
    }
    else if (iswalpha(*expressionPointer)) {
        while (!isDelimiter(*expressionPointer))
            *temp++ = *expressionPointer++;
        tokenType = VARIABLE;
    }
    else if (iswdigit(*expressionPointer)) {
        while (!isDelimiter(*expressionPointer))
            *temp++ = *expressionPointer++;
        tokenType = NUMBER;
    }
    *temp = L'\0';
}

// Метод для перевірки, чи є символ роздільником
bool ExpressionParser::isDelimiter(wchar_t c) {
    return wcschr(L" +-/*%^=()", c) || c == 9 || c == L'\r' || c == 0;
}

// Метод для пошуку значення змінної
double ExpressionParser::findVariable(const wchar_t* var) {
    std::wstring varName(var);
    if (variables.find(varName) == variables.end()) {
        syntaxError(0);  // невідома змінна
        return 0.0;
    }
    return variables[varName];
}

// Метод для присвоєння значення змінній
void ExpressionParser::assignVariable(const wchar_t* var, double value) {
    std::wstring varName(var);
    variables[varName] = value;
}

// Метод для виведення повідомлень про синтаксичні помилки
void ExpressionParser::syntaxError(int errorCode) {
    static const wchar_t* errors[] = {
        L"Синтаксична помилка",
        L"Незакриті дужки",
        L"Немає виразу"
    };
    std::wcout << errors[errorCode] << std::endl;
}

// Метод для обчислення похідних
std::wstring ExpressionParser::differentiate(wchar_t* expression, const std::wstring& variable) {
    expressionPointer = expression;
    parseToken();
    return differentiateTerm();
}

std::wstring ExpressionParser::differentiateTerm() {
    std::wstring result = differentiateFactor();
    wchar_t op;
    while ((op = *currentToken) == L'+' || op == L'-') {
        parseToken();
        result += op;
        result += differentiateFactor();
    }
    return result;
}

std::wstring ExpressionParser::differentiateFactor() {
    std::wstring result = differentiatePower();
    wchar_t op;
    while ((op = *currentToken) == L'*' || op == L'/' || op == L'%') {
        parseToken();
        std::wstring temp = differentiatePower();
        if (op == L'*') {
            result = L"(" + result + L"*" + temp + L")";
        }
        else {
            result += op;
            result += temp;
        }
    }
    return result;
}

std::wstring ExpressionParser::differentiatePower() {
    std::wstring base = differentiateUnary();
    if (*currentToken == L'^') {
        parseToken();
        std::wstring exponent = differentiateUnary();
        std::wstringstream ss;
        ss << exponent << L"*" << base << L"^(" << exponent << L"-1)";
        return ss.str();
    }
    return base;
}

std::wstring ExpressionParser::differentiateUnary() {
    wchar_t op = 0;
    if ((tokenType == DELIMITER) && (*currentToken == L'+' || *currentToken == L'-')) {
        op = *currentToken;
        parseToken();
    }
    std::wstring result = differentiatePrimary();
    if (op == L'-')
        result = L"-" + result;
    return result;
}

std::wstring ExpressionParser::differentiatePrimary() {
    std::wstring result;
    if (*currentToken == L'(') {
        parseToken();
        result = L"(" + differentiateTerm() + L")";
        if (*currentToken != L')')
            syntaxError(1);
        parseToken();
    }
    else {
        switch (tokenType) {
        case NUMBER:
            result = L"0";
            parseToken();
            break;
        case VARIABLE: {
            wchar_t tempToken[80];
            wcscpy_s(tempToken, currentToken);
            result = differentiateVariable(tempToken, L"x");
            parseToken();
            break;
        }
        default:
            syntaxError(0);
        }
    }
    return result;
}

std::wstring ExpressionParser::differentiateVariable(const std::wstring& var, const std::wstring& diffVar) {
    if (var == diffVar) {
        return L"1";
    }
    return L"0";
}

// Метод для обчислення інтегралів
std::wstring ExpressionParser::integrate(wchar_t* expression, const std::wstring& variable) {
    expressionPointer = expression;
    parseToken();
    return integrateTerm();
}

std::wstring ExpressionParser::integrateTerm() {
    std::wstring result = integrateFactor();
    wchar_t op;
    while ((op = *currentToken) == L'+' || op == L'-') {
        parseToken();
        result += op;
        result += integrateFactor();
    }
    return result;
}

std::wstring ExpressionParser::integrateFactor() {
    std::wstring result = integratePower();
    wchar_t op;
    while ((op = *currentToken) == L'*' || op == L'/' || op == L'%') {
        parseToken();
        std::wstring temp = integratePower();
        result += op;
        result += temp;
    }
    return result;
}

std::wstring ExpressionParser::integratePower() {
    std::wstring base = integrateUnary();
    if (*currentToken == L'^') {
        parseToken();
        std::wstring exponent = integrateUnary();
        std::wstringstream ss;
        ss << L"(" << base << L"^(" << exponent << L"+1))/(" << exponent << L"+1)";
        return ss.str();
    }
    return base;
}

std::wstring ExpressionParser::integrateUnary() {
    wchar_t op = 0;
    if ((tokenType == DELIMITER) && (*currentToken == L'+' || *currentToken == L'-')) {
        op = *currentToken;
        parseToken();
    }
    std::wstring result = integratePrimary();
    if (op == L'-')
        result = L"-" + result;
    return result;
}

std::wstring ExpressionParser::integratePrimary() {
    std::wstring result;
    if (*currentToken == L'(') {
        parseToken();
        result = L"(" + integrateTerm() + L")";
        if (*currentToken != L')')
            syntaxError(1);
        parseToken();
    }
    else {
        switch (tokenType) {
        case NUMBER: {
            wchar_t tempToken[80];
            wcscpy_s(tempToken, currentToken);
            result = tempToken;
            parseToken();
            break;
        }
        case VARIABLE: {
            wchar_t tempToken[80];
            wcscpy_s(tempToken, currentToken);
            result = integrateVariable(tempToken, L"x");
            parseToken();
            break;
        }
        default:
            syntaxError(0);
        }
    }
    return result;
}

std::wstring ExpressionParser::integrateVariable(const std::wstring& var, const std::wstring& intVar) {
    if (var == intVar) {
        return L"0.5*" + intVar + L"^2";
    }
    return var + L"*" + intVar;
}

int main() {
    // Встановлення локалі для підтримки кирилиці
    std::setlocale(LC_ALL, "uk_UA.UTF-8");

    // Налаштування консолі для підтримки UTF-8
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    wchar_t expression[80];
    ExpressionParser parser;

    std::wcout << L"Для виходу введіть крапку.\n";
    std::wcout << L"Щоб обчислити похідну, введіть команду: d/dx <вираз>\n";
    std::wcout << L"Щоб обчислити інтеграл, введіть команду: int <вираз>\n";

    for (;;) {
        std::wcout << L"Введіть команду або вираз: ";
        std::wcin.getline(expression, 79);
        if (*expression == L'.')
            break;

        // Перевірка на команди для похідної або інтегралу
        if (wcsncmp(expression, L"d/dx ", 5) == 0) {
            std::wcout << L"Похідна: " << parser.differentiate(expression + 5, L"x") << L"\n\n";
        }
        else if (wcsncmp(expression, L"int ", 4) == 0) {
            std::wcout << L"Інтеграл: " << parser.integrate(expression + 4, L"x") << L"\n\n";
        }
        else {
            std::wcout << L"Відповідь: " << parser.evaluateExpression(expression) << L"\n\n";
        }
    }
    return 0;
}
