/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "csv_parser.h"

#include <algorithm>
#include <cctype>

META_BEGIN_NAMESPACE()

CsvParser::CsvParser(BASE_NS::string_view csv, const char delimiter) : delimiter_(delimiter), csv_(csv) {}

bool CsvParser::GetRow(CsvRow& row)
{
    auto nextRow = ParseRow();
    row.swap(nextRow);
    return !row.empty();
}

void CsvParser::Reset()
{
    pos_ = 0;
}

/**
 * @brief Returns a trimmed string based on state.
 * @param sv The string to trim.
 * @param state State of the parser.
 * @return If state is QUOTED, returns the string itself. Otherwise returns the string
 *         trimmed from trailing and leading whitespace.
 */
BASE_NS::string_view CsvParser::Trimmed(BASE_NS::string_view sv, State state)
{
    if (state == QUOTED) {
        return sv;
    }
    constexpr auto nspace = [](unsigned char ch) { return !std::isspace(static_cast<int>(ch)); };
    sv.remove_suffix(std::distance(std::find_if(sv.rbegin(), sv.rend(), nspace).base(), sv.end()));
    sv.remove_prefix(std::find_if(sv.begin(), sv.end(), nspace) - sv.begin());
    return sv;
}

std::pair<bool, char> HandleEscaped(char next)
{
    std::pair<bool, char> result { true, next };
    switch (next) {
        case 'n':
            result.second = '\n';
            break;
        case '\\':
            result.second = '\\';
            break;
        case 't':
            result.second = '\t';
            break;
        case '"':
            result.second = '"';
            break;
        default:
            result.first = false;
            break;
    }
    return result;
}

CsvParser::CsvRow CsvParser::ParseRow()
{
    BASE_NS::vector<BASE_NS::string> items;
    BASE_NS::string item;
    State state { NO_QUOTE };

    while (pos_ < csv_.size()) {
        auto c = csv_[pos_++];
        if (c == '\r') { // Ignore carriage returns
            continue;
        }
        if (c == '"') {
            if (state == IN_QUOTE && pos_ < csv_.size() - 1 && csv_[pos_] == '"') {
                // Double quotes interpreted as a single quote
                item += c;
                pos_++;
            } else { // Begin/end quote
                state = (state == NO_QUOTE) ? IN_QUOTE : QUOTED;
                if (state == IN_QUOTE) {
                    // Quoted part starts, ignore anything before it
                    item.clear();
                }
            }
        } else if (c == delimiter_ && state != IN_QUOTE) {
            // Delimiter found while not within quotes, move to next item
            items.emplace_back(Trimmed(item, state));
            item.clear();
            state = NO_QUOTE;
        } else if (c == '\n' && state != IN_QUOTE) {
            // End of line while not within quotes, the row is complete
            break;
        } else if (state != QUOTED) {
            // By default include character in result, unless we already had
            // quoted content, then anything outside of quotes is ignored until
            // next delimiter
            if (c == '\\' && pos_ < csv_.size() - 1) {
                if (auto esc = HandleEscaped(csv_[pos_]); esc.first) {
                    item += esc.second;
                    pos_++;
                    continue;
                }
            }
            item += c;
        }
    }

    // Any leftover since the last delimiter is the last item on the row
    if (auto trimmed = Trimmed(item, state); !trimmed.empty()) {
        items.emplace_back(trimmed);
    }
    return items;
}

META_END_NAMESPACE()
