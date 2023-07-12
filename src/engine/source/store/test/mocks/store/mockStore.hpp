#ifndef _STORE_MOCK_STORE_HPP
#define _STORE_MOCK_STORE_HPP

#include <gmock/gmock.h>

#include <store/istore.hpp>

namespace store::mocks
{

std::variant<json::Json, base::Error> getError = base::Error {"Mocked get error"};
inline std::variant<json::Json, base::Error> getSuccess(json::Json& expected)
{
    return expected;
}

std::optional<base::Error> addError = base::Error {"Mocked add error"};
std::optional<base::Error> addSuccess = std::nullopt;

std::optional<base::Error> delError = base::Error {"Mocked del error"};
std::optional<base::Error> delSuccess = std::nullopt;

std::optional<base::Error> updateError = base::Error {"Mocked update error"};
std::optional<base::Error> updateSuccess = std::nullopt;

std::optional<base::Error> addUpdateError = base::Error {"Mocked addUpdate error"};
std::optional<base::Error> addUpdateSuccess = std::nullopt;

class MockStoreRead : public store::IStoreRead
{
public:
    MOCK_METHOD((std::variant<json::Json, base::Error>), get, (const base::Name&), (const, override));
};

class MockStore : public store::IStore
{
public:
    MOCK_METHOD((std::variant<json::Json, base::Error>), get, (const base::Name&), (const, override));
    MOCK_METHOD((std::optional<base::Error>), add, (const base::Name&, const json::Json&), (override));
    MOCK_METHOD((std::optional<base::Error>), del, (const base::Name&), (override));
    MOCK_METHOD((std::optional<base::Error>), update, (const base::Name&, const json::Json&), (override));
    MOCK_METHOD((std::optional<base::Error>), addUpdate, (const base::Name&, const json::Json&), (override));
};

} // namespace store::mocks

#endif // _STORE_MOCK_STORE_HPP
