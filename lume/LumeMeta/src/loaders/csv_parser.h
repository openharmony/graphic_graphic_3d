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

#ifndef META_SRC_LOADERS_CSV_PARSER_H
#define META_SRC_LOADERS_CSV_PARSER_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The CsvParser class implements a simple CSV parser, which
 *        parses a given CSV format string.
 *        The parser supports non-quoted and quoted items, separated
 *        by a configurable delimiter. Also multiline quotes are
 *        supported.
 */
class CsvParser {
public:
    CsvParser() = delete;
    explicit CsvParser(BASE_NS::string_view csv, const char delimiter = ',');

    using CsvRow = BASE_NS::vector<BASE_NS::string>;

    /**
     * @brief GetRow returns the next row from the CSV file.
     *        Regularly the user should call GetRow in a loop until
     *        the function returns false (either parse error or
     *        end of the CSV file).
     * @param row Row items will be placed here.
     * @return True if parsing was successful, false otherwide.
     */
    bool GetRow(CsvRow& row);
    /**
     * @brief Resets to the beginning of the CSV string.
     */
    void Reset();

private:
    enum State {
        NO_QUOTE,
        IN_QUOTE,
        QUOTED,
    };
    CsvRow ParseRow();
    BASE_NS::string_view Trimmed(BASE_NS::string_view sv, State state);

    char delimiter_ {};
    BASE_NS::string csv_;
    size_t pos_ {};
};

META_END_NAMESPACE()

#endif
