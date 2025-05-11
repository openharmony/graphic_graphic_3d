/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: CSV Parser
 * Author: Lauri Jaaskela
 * Create: 2022-09-05
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
