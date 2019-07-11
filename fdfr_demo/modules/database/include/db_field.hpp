#include "db_types.hpp"
#include "optional.hpp"

namespace database {

template <data_type Type, size_t Length>
struct Field {
    using type = std::integral_constant<data_type, Type>;
    using length = std::integral_constant<size_t, Length>;
};

template <size_t Index, typename F>
class EntryElement;

template <typename... Fields>
class Entry;

template <>
class Entry<> {
public:
    static size_t bytecount_of(size_t index) {
        throw std::runtime_error("index of read exceeds the number of field count.");
    }
    static data_type type_of(size_t index) {
        throw std::runtime_error("index of read exceeds the number of field count.");
    }
    static size_t length_of(size_t index) {
        throw std::runtime_error("index of read exceeds the number of field count.");
    }
    size_t read(std::istream &fin, size_t index) {
        throw std::runtime_error("index of read exceeds the number of field count.");
    }
    void write(std::ostream &fout, size_t index) {
        throw std::runtime_error("index of read exceeds the number of field count.");
    }
    mp::optional<size_t> file_pos(size_t index) const {
        throw std::runtime_error("index of read exceeds the number of field count.");
    }
    void set_file_pos(size_t index, mp::optional<size_t> pos) {
        throw std::runtime_error("index of read exceeds the number of field count.");
    }
};

template <typename Field, typename... Fields>
class Entry<Field, Fields...> {
    template <size_t I, typename Fs>
    friend class EntryElement;
    using field_t = std::vector<typename data<Field::type::value>::type>;
public:
    Entry() : content(Field::length::value) {}
    Entry(const Entry &) = default;
    Entry(Entry &&) = default;
    Entry &operator=(const Entry &) = default;
    Entry &operator=(Entry &&) = default;

    static size_t bytecount_of(size_t index) {
        if (index == 0) {
            return sizeof(typename data<Field::type::value>::type) * Field::length::value;
        } else {
            return Entry<Fields...>::bytecount_of(index - 1);
        }
    }

    static data_type type_of(size_t index) {
        if (index == 0) {
            return Field::type::value;
        } else {
            return Entry<Fields...>::type_of(index - 1);
        }
    }

    static size_t length_of(size_t index) {
        if (index == 0) {
            return Field::length::value;
        } else {
            return Entry<Fields...>::length_of(index - 1);
        }
    }

    mp::optional<size_t> file_pos(size_t index) const {
        if (index == 0) {
            return file_pos_;
        } else {
            return other_fields.file_pos(index - 1);
        }
    }

    void set_file_pos(size_t index, mp::optional<size_t> pos) {
        if (index == 0) {
            file_pos_ = pos;
        } else {
            other_fields.set_file_pos(index - 1, pos);
        }
    }

    size_t read(std::istream &fin, size_t index) {
        if (index == 0) {
            file_pos_.emplace(fin.tellg());
            auto size = this->bytecount_of(0);

            fin.read(reinterpret_cast<char *>(content.data()), size);

            if (fin.gcount() != size) {
                throw std::ios_base::failure("");
            }
        } else {
            return other_fields.read(fin, index - 1);
        }
    }

    void write(std::ostream &fout, size_t index) {
        if (index == 0) {
            file_pos_.emplace(fout.tellp());

            auto size = this->bytecount_of(0);
            fout.write(reinterpret_cast<char *>(content.data()), size);
        } else {
            return other_fields.write(fout, index - 1);
        }
    }

private:
    field_t content;
    mp::optional<size_t> file_pos_;
    Entry<Fields...> other_fields;
};

template <size_t Index, typename Field, typename... Fields>
class EntryElement<Index, Entry<Field, Fields...>> {
public:
    using field_t = typename EntryElement<Index - 1, Entry<Fields...>>::field_t;
    static inline field_t &get(Entry<Field, Fields...> &fields) {
        return EntryElement<Index - 1, Entry<Fields...>>::get(fields.other_fields);
    }
};

template <typename Field, typename... Fields>
class EntryElement<0, Entry<Field, Fields...>> {
public:
    using field_t = typename Entry<Field, Fields...>::field_t;
    static inline field_t &get(Entry<Field, Fields...> &fields) {
        return fields.content;
    }
};

template <size_t Index, typename Fs>
typename EntryElement<Index, Fs>::field_t &get(Fs &fields) {
    return EntryElement<Index, Fs>::get(fields);
}

}