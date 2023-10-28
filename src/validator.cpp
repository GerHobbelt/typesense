#include "validator.h"

Option<uint32_t> validator_t::coerce_element(const field& a_field, nlohmann::json& document,
                                       nlohmann::json& doc_ele,
                                       const std::string& fallback_field_type,
                                       const DIRTY_VALUES& dirty_values) {

    const std::string& field_name = a_field.name;
    bool array_ele_erased = false;
    nlohmann::json::iterator dummy_iter;

    if(a_field.type == field_types::STRING && !doc_ele.is_string()) {
        Option<uint32_t> coerce_op = coerce_string(dirty_values, fallback_field_type, a_field, document, field_name, dummy_iter, false, array_ele_erased);
        if(!coerce_op.ok()) {
            return coerce_op;
        }
    } else if(a_field.type == field_types::INT32) {
        if(!doc_ele.is_number_integer()) {
            Option<uint32_t> coerce_op = coerce_int32_t(dirty_values, a_field, document, field_name, dummy_iter, false, array_ele_erased);
            if(!coerce_op.ok()) {
                return coerce_op;
            }
        }
    } else if(a_field.type == field_types::INT64 && !doc_ele.is_number_integer()) {
        Option<uint32_t> coerce_op = coerce_int64_t(dirty_values, a_field, document, field_name, dummy_iter, false, array_ele_erased);
        if(!coerce_op.ok()) {
            return coerce_op;
        }
    } else if(a_field.type == field_types::FLOAT && !doc_ele.is_number()) {
        // using `is_number` allows integer to be passed to a float field
        Option<uint32_t> coerce_op = coerce_float(dirty_values, a_field, document, field_name, dummy_iter, false, array_ele_erased);
        if(!coerce_op.ok()) {
            return coerce_op;
        }
    } else if(a_field.type == field_types::BOOL && !doc_ele.is_boolean()) {
        Option<uint32_t> coerce_op = coerce_bool(dirty_values, a_field, document, field_name, dummy_iter, false, array_ele_erased);
        if(!coerce_op.ok()) {
            return coerce_op;
        }
    } else if(a_field.type == field_types::GEOPOINT) {
        if(!doc_ele.is_array() || doc_ele.size() != 2) {
            return Option<>(400, "Field `" + field_name  + "` must be a 2 element array: [lat, lng].");
        }

        if(!(doc_ele[0].is_number() && doc_ele[1].is_number())) {
            // one or more elements is not an number, try to coerce
            Option<uint32_t> coerce_op = coerce_geopoint(dirty_values, a_field, document, field_name, dummy_iter, false, array_ele_erased);
            if(!coerce_op.ok()) {
                return coerce_op;
            }
        }
    } else if(a_field.is_array()) {
        if(!doc_ele.is_array()) {
            if(a_field.optional && (dirty_values == DIRTY_VALUES::DROP ||
                                    dirty_values == DIRTY_VALUES::COERCE_OR_DROP)) {
                document.erase(field_name);
                return Option<uint32_t>(200);
            } else {
                return Option<>(400, "Field `" + field_name  + "` must be an array.");
            }
        }

        nlohmann::json::iterator it = doc_ele.begin();

        // Handle a geopoint[] type inside an array of object: it won't be an array of array, so cannot iterate
        if(a_field.nested && a_field.type == field_types::GEOPOINT_ARRAY &&
           it->is_number() && doc_ele.size() == 2) {
            const auto& item = doc_ele;
            if(!(item[0].is_number() && item[1].is_number())) {
                // one or more elements is not an number, try to coerce
                Option<uint32_t> coerce_op = coerce_geopoint(dirty_values, a_field, document, field_name, it, true, array_ele_erased);
                if(!coerce_op.ok()) {
                    return coerce_op;
                }
            }

            return Option<uint32_t>(200);
        }

        for(; it != doc_ele.end(); ) {
            const auto& item = it.value();
            array_ele_erased = false;

            if (a_field.type == field_types::STRING_ARRAY && !item.is_string()) {
                Option<uint32_t> coerce_op = coerce_string(dirty_values, fallback_field_type, a_field, document, field_name, it, true, array_ele_erased);
                if (!coerce_op.ok()) {
                    return coerce_op;
                }
            } else if (a_field.type == field_types::INT32_ARRAY && !item.is_number_integer()) {
                Option<uint32_t> coerce_op = coerce_int32_t(dirty_values, a_field, document, field_name, it, true, array_ele_erased);
                if (!coerce_op.ok()) {
                    return coerce_op;
                }
            } else if (a_field.type == field_types::INT64_ARRAY && !item.is_number_integer()) {
                Option<uint32_t> coerce_op = coerce_int64_t(dirty_values, a_field, document, field_name, it, true, array_ele_erased);
                if (!coerce_op.ok()) {
                    return coerce_op;
                }
            } else if (a_field.type == field_types::FLOAT_ARRAY && !item.is_number()) {
                // we check for `is_number` to allow whole numbers to be passed into float fields
                Option<uint32_t> coerce_op = coerce_float(dirty_values, a_field, document, field_name, it, true, array_ele_erased);
                if (!coerce_op.ok()) {
                    return coerce_op;
                }
            } else if (a_field.type == field_types::BOOL_ARRAY && !item.is_boolean()) {
                Option<uint32_t> coerce_op = coerce_bool(dirty_values, a_field, document, field_name, it, true, array_ele_erased);
                if (!coerce_op.ok()) {
                    return coerce_op;
                }
            } else if (a_field.type == field_types::GEOPOINT_ARRAY) {
                if(!item.is_array() || item.size() != 2) {
                    return Option<>(400, "Field `" + field_name  + "` must contain 2 element arrays: [ [lat, lng],... ].");
                }

                if(!(item[0].is_number() && item[1].is_number())) {
                    // one or more elements is not an number, try to coerce
                    Option<uint32_t> coerce_op = coerce_geopoint(dirty_values, a_field, document, field_name, it, true, array_ele_erased);
                    if(!coerce_op.ok()) {
                        return coerce_op;
                    }
                }
            }

            if(!array_ele_erased) {
                // if it is erased, the iterator will be reassigned
                it++;
            }
        }
    }

    return Option<uint32_t>(200);
}

Option<uint32_t> validator_t::coerce_string(const DIRTY_VALUES& dirty_values, const std::string& fallback_field_type,
                                      const field& a_field, nlohmann::json &document,
                                      const std::string &field_name, nlohmann::json::iterator& array_iter,
                                      bool is_array, bool& array_ele_erased) {
    std::string suffix = is_array ? "an array of" : "a";
    auto& item = is_array ? array_iter.value() : document[field_name];

    if(dirty_values == DIRTY_VALUES::REJECT) {
        return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " string.");
    }

    if(dirty_values == DIRTY_VALUES::DROP) {
        if(!a_field.optional) {
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " string.");
        }

        if(!is_array) {
            document.erase(field_name);
        } else {
            array_iter = document[field_name].erase(array_iter);
            array_ele_erased = true;
        }
        return Option<uint32_t>(200);
    }

    // we will try to coerce the value to a string

    if (item.is_number_integer()) {
        item = std::to_string((int64_t)item);
    }

    else if(item.is_number_float()) {
        item = StringUtils::float_to_str((float)item);
    }

    else if(item.is_boolean()) {
        item = item == true ? "true" : "false";
    }

    else {
        if(dirty_values == DIRTY_VALUES::COERCE_OR_DROP) {
            if(!a_field.optional) {
                if(a_field.nested && item.is_array()) {
                    return Option<>(400, "Field `" + field_name + "` has an incorrect type. "
                                                                  "Hint: field inside an array of objects must be an array type as well.");
                }
                return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " string.");
            }

            if(!is_array) {
                document.erase(field_name);
            } else {
                array_iter = document[field_name].erase(array_iter);
                array_ele_erased = true;
            }
        } else {
            // COERCE_OR_REJECT / non-optional + DROP
            if(a_field.nested && item.is_array()) {
                return Option<>(400, "Field `" + field_name + "` has an incorrect type. "
                                                              "Hint: field inside an array of objects must be an array type as well.");
            }
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " string.");
        }
    }

    return Option<>(200);
}

Option<uint32_t> validator_t::coerce_int32_t(const DIRTY_VALUES& dirty_values, const field& a_field, nlohmann::json &document,
                                       const std::string &field_name,
                                       nlohmann::json::iterator& array_iter, bool is_array, bool& array_ele_erased) {
    std::string suffix = is_array ? "an array of" : "an";
    auto& item = is_array ? array_iter.value() : document[field_name];

    if(dirty_values == DIRTY_VALUES::REJECT) {
        return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " int32.");
    }

    if(dirty_values == DIRTY_VALUES::DROP) {
        if(!a_field.optional) {
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " int32.");
        }

        if(!is_array) {
            document.erase(field_name);
        } else {
            array_iter = document[field_name].erase(array_iter);
            array_ele_erased = true;
        }

        return Option<uint32_t>(200);
    }

    // try to value coerce into an integer

    if(item.is_number_float()) {
        item = static_cast<int32_t>(item.get<float>());
    }

    else if(item.is_boolean()) {
        item = item == true ? 1 : 0;
    }

    else if(item.is_string() && StringUtils::is_int32_t(item)) {
        item = std::atol(item.get<std::string>().c_str());
    }

    else {
        if(dirty_values == DIRTY_VALUES::COERCE_OR_DROP) {
            if(!a_field.optional) {
                return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " int32.");
            }

            if(!is_array) {
                document.erase(field_name);
            } else {
                array_iter = document[field_name].erase(array_iter);
                array_ele_erased = true;
            }
        } else {
            // COERCE_OR_REJECT / non-optional + DROP
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " int32.");
        }
    }

    if(document.contains(field_name) && document[field_name].get<int64_t>() > INT32_MAX) {
        if(a_field.optional && (dirty_values == DIRTY_VALUES::DROP || dirty_values == DIRTY_VALUES::COERCE_OR_REJECT)) {
            document.erase(field_name);
        } else {
            return Option<>(400, "Field `" + field_name  + "` exceeds maximum value of int32.");
        }
    }

    return Option<uint32_t>(200);
}

Option<uint32_t> validator_t::coerce_int64_t(const DIRTY_VALUES& dirty_values, const field& a_field, nlohmann::json &document,
                                       const std::string &field_name,
                                       nlohmann::json::iterator& array_iter, bool is_array, bool& array_ele_erased) {
    std::string suffix = is_array ? "an array of" : "an";
    auto& item = is_array ? array_iter.value() : document[field_name];

    if(dirty_values == DIRTY_VALUES::REJECT) {
        return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " int64.");
    }

    if(dirty_values == DIRTY_VALUES::DROP) {
        if(!a_field.optional) {
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " int64.");
        }

        if(!is_array) {
            document.erase(field_name);
        } else {
            array_iter = document[field_name].erase(array_iter);
            array_ele_erased = true;
        }
        return Option<uint32_t>(200);
    }

    // try to value coerce into an integer

    if(item.is_number_float()) {
        item = static_cast<int64_t>(item.get<float>());
    }

    else if(item.is_boolean()) {
        item = item == true ? 1 : 0;
    }

    else if(item.is_string() && StringUtils::is_int64_t(item)) {
        item = std::atoll(item.get<std::string>().c_str());
    }

    else {
        if(dirty_values == DIRTY_VALUES::COERCE_OR_DROP) {
            if(!a_field.optional) {
                return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " int64.");
            }

            if(!is_array) {
                document.erase(field_name);
            } else {
                array_iter = document[field_name].erase(array_iter);
                array_ele_erased = true;
            }
        } else {
            // COERCE_OR_REJECT / non-optional + DROP
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " int64.");
        }
    }

    return Option<uint32_t>(200);
}

Option<uint32_t> validator_t::coerce_bool(const DIRTY_VALUES& dirty_values, const field& a_field, nlohmann::json &document,
                                    const std::string &field_name,
                                    nlohmann::json::iterator& array_iter, bool is_array, bool& array_ele_erased) {
    std::string suffix = is_array ? "a array of" : "a";
    auto& item = is_array ? array_iter.value() : document[field_name];

    if(dirty_values == DIRTY_VALUES::REJECT) {
        return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " bool.");
    }

    if(dirty_values == DIRTY_VALUES::DROP) {
        if(!a_field.optional) {
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " bool.");
        }

        if(!is_array) {
            document.erase(field_name);
        } else {
            array_iter = document[field_name].erase(array_iter);
            array_ele_erased = true;
        }
        return Option<uint32_t>(200);
    }

    // try to value coerce into a bool
    if (item.is_number_integer() &&
        (item.get<int64_t>() == 1 || item.get<int64_t>() == 0)) {
        item = item.get<int64_t>() == 1;
    }

    else if(item.is_string()) {
        std::string str_val = item.get<std::string>();
        StringUtils::tolowercase(str_val);
        if(str_val == "true") {
            item = true;
            return Option<uint32_t>(200);
        } else if(str_val == "false") {
            item = false;
            return Option<uint32_t>(200);
        } else {
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " bool.");
        }
    }

    else {
        if(dirty_values == DIRTY_VALUES::COERCE_OR_DROP) {
            if(!a_field.optional) {
                return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " bool.");
            }

            if(!is_array) {
                document.erase(field_name);
            } else {
                array_iter = document[field_name].erase(array_iter);
                array_ele_erased = true;
            }
        } else {
            // COERCE_OR_REJECT / non-optional + DROP
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " bool.");
        }
    }

    return Option<uint32_t>(200);
}

Option<uint32_t> validator_t::coerce_geopoint(const DIRTY_VALUES& dirty_values, const field& a_field, nlohmann::json &document,
                                        const std::string &field_name,
                                        nlohmann::json::iterator& array_iter, bool is_array, bool& array_ele_erased) {
    std::string suffix = is_array ? "an array of" : "a";
    auto& item = is_array ? array_iter.value() : document[field_name];

    if(dirty_values == DIRTY_VALUES::REJECT) {
        return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " geopoint.");
    }

    if(dirty_values == DIRTY_VALUES::DROP) {
        if(!a_field.optional) {
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " geopoint.");
        }

        if(!is_array) {
            document.erase(field_name);
        } else {
            array_iter = document[field_name].erase(array_iter);
            array_ele_erased = true;
        }
        return Option<uint32_t>(200);
    }

    // try to value coerce into a geopoint

    if(!item[0].is_number() && item[0].is_string()) {
        if(StringUtils::is_float(item[0])) {
            item[0] = std::stof(item[0].get<std::string>());
        }
    }

    if(!item[1].is_number() && item[1].is_string()) {
        if(StringUtils::is_float(item[1])) {
            item[1] = std::stof(item[1].get<std::string>());
        }
    }

    if(!item[0].is_number() || !item[1].is_number()) {
        if(dirty_values == DIRTY_VALUES::COERCE_OR_DROP) {
            if(!a_field.optional) {
                return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " geopoint.");
            }

            if(!is_array) {
                document.erase(field_name);
            } else {
                array_iter = document[field_name].erase(array_iter);
                array_ele_erased = true;
            }
        } else {
            // COERCE_OR_REJECT / non-optional + DROP
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " geopoint.");
        }
    }

    return Option<uint32_t>(200);
}

Option<uint32_t> validator_t::coerce_float(const DIRTY_VALUES& dirty_values, const field& a_field, nlohmann::json &document,
                                     const std::string &field_name,
                                     nlohmann::json::iterator& array_iter, bool is_array, bool& array_ele_erased) {
    std::string suffix = is_array ? "a array of" : "a";
    auto& item = is_array ? array_iter.value() : document[field_name];

    if(dirty_values == DIRTY_VALUES::REJECT) {
        return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " float.");
    }

    if(dirty_values == DIRTY_VALUES::DROP) {
        if(!a_field.optional) {
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " float.");
        }

        if(!is_array) {
            document.erase(field_name);
        } else {
            array_iter = document[field_name].erase(array_iter);
            array_ele_erased = true;
        }
        return Option<uint32_t>(200);
    }

    // try to value coerce into a float

    if(item.is_string() && StringUtils::is_float(item)) {
        item = std::atof(item.get<std::string>().c_str());
    }

    else if(item.is_boolean()) {
        item = item == true ? 1.0 : 0.0;
    }

    else {
        if(dirty_values == DIRTY_VALUES::COERCE_OR_DROP) {
            if(!a_field.optional) {
                return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " float.");
            }

            if(!is_array) {
                document.erase(field_name);
            } else {
                array_iter = document[field_name].erase(array_iter);
                array_ele_erased = true;
            }
        } else {
            // COERCE_OR_REJECT / non-optional + DROP
            return Option<>(400, "Field `" + field_name  + "` must be " + suffix + " float.");
        }
    }

    return Option<uint32_t>(200);
}

Option<uint32_t> validator_t::validate_index_in_memory(nlohmann::json& document, uint32_t seq_id,
                                                 const std::string & default_sorting_field,
                                                 const tsl::htrie_map<char, field> & search_schema,
                                                 const index_operation_t op,
                                                 const std::string& fallback_field_type,
                                                 const DIRTY_VALUES& dirty_values) {

    bool missing_default_sort_field = (!default_sorting_field.empty() && document.count(default_sorting_field) == 0);

    if((op != UPDATE && op != EMPLACE) && missing_default_sort_field) {
        return Option<>(400, "Field `" + default_sorting_field  + "` has been declared as a default sorting field, "
                                                                  "but is not found in the document.");
    }

    for(const auto& a_field: search_schema) {
        const std::string& field_name = a_field.name;

        if(field_name == "id" || a_field.is_object()) {
            continue;
        }

        if((a_field.optional || op == UPDATE || op == EMPLACE) && document.count(field_name) == 0) {
            continue;
        }

        if(document.count(field_name) == 0) {
            return Option<>(400, "Field `" + field_name  + "` has been declared in the schema, "
                                                           "but is not found in the document.");
        }

        nlohmann::json& doc_ele = document[field_name];

        if(a_field.optional && doc_ele.is_null()) {
            // we will ignore `null` on an option field
            if(op != UPDATE && op != EMPLACE) {
                // for updates, the erasure is done later since we need to keep the key for overwrite
                document.erase(field_name);
            }
            continue;
        }

        auto coerce_op = coerce_element(a_field, document, doc_ele, fallback_field_type, dirty_values);
        if(!coerce_op.ok()) {
            return coerce_op;
        }
    }

    return Option<>(200);
}