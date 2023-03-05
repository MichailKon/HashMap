#ifndef ROBINHOODHASING_HASHMAP_H
#define ROBINHOODHASING_HASHMAP_H

#include <functional>
#include <stdexcept>
#include <list>
#include <cassert>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    using HashMapItem = typename std::pair<const KeyType, ValueType>;

    class iterator {
    public:
        iterator() : item(nullptr), pos(0), sz(0) {}

        iterator(std::pair<HashMapItem *, size_t> *item, size_t pos, size_t sz) : item(item), pos(pos), sz(sz) {}

        HashMapItem &operator*() const {
            return *item->first;
        }

        HashMapItem *operator->() const {
            return item->first;
        }

        iterator &operator++() {
            if (pos == sz) {
                return *this;
            }
            do {
                ++pos;
                ++item;
            } while (pos < sz && item->first == nullptr);
            return *this;
        }

        iterator operator++(int) {
            if (pos == sz) {
                return *this;
            }
            iterator was = *this;
            do {
                ++pos;
                ++item;
            } while (pos < sz && item->first == nullptr);
            return was;
        }

        iterator &operator--() {
            if (pos == 0) {
                return *this;
            }
            do {
                --pos;
                --item;
            } while (item->first == nullptr && pos != 0);
            return *this;
        }

        iterator operator--(int) {
            if (pos == 0) {
                return *this;
            }
            iterator was = *this;
            do {
                --pos;
                --item;
            } while (item->first == nullptr && pos != 0);
            return was;
        }

        bool operator==(const iterator &other) const {
            return item == other.item && pos == other.pos && sz == other.sz;
        }

        bool operator!=(const iterator &other) const {
            return !(item == other.item && pos == other.pos && sz == other.sz);
        }

    private:
        std::pair<HashMapItem *, size_t> *item;
        size_t pos;
        size_t sz;
    };

    class const_iterator {
    public:
        const_iterator() : item(nullptr), pos(0), sz(0) {}

        const_iterator(const std::pair<HashMapItem *, size_t> *item, size_t pos, size_t sz) : item(item), pos(pos),
                                                                                              sz(sz) {}

        HashMapItem &operator*() const {
            return *item->first;
        }

        HashMapItem *operator->() const {
            return item->first;
        }

        const_iterator &operator++() {
            if (pos == sz) {
                return *this;
            }
            do {
                ++pos;
                ++item;
            } while (pos < sz && item->first == nullptr);
            return *this;
        }

        const_iterator operator++(int) {
            if (pos == sz) {
                return *this;
            }
            const_iterator was = *this;
            do {
                ++pos;
                ++item;
            } while (pos < sz && item->first == nullptr);
            return was;
        }

        const_iterator &operator--() {
            if (pos == 0) {
                return *this;
            }
            do {
                --pos;
                --item;
            } while (item->first == nullptr && pos != 0);
            return *this;
        }

        const_iterator operator--(int) {
            if (pos == 0) {
                return *this;
            }
            const_iterator was = *this;
            do {
                --pos;
                --item;
            } while (item->first == nullptr && pos != 0);
            return was;
        }

        bool operator==(const const_iterator &other) const {
            return item == other.item && pos == other.pos && sz == other.sz;
        }

        bool operator!=(const const_iterator &other) const {
            return !(item == other.item && pos == other.pos && sz == other.sz);
        }

    private:
        const std::pair<HashMapItem *, size_t> *item;
        size_t pos;
        size_t sz;
    };

    ~HashMap() {
        for (auto &i: arr_) {
            if (i.first != nullptr) {
                delete i.first;
            }
        }
    }

    HashMap() : hasher_(Hash()) {
        arr_.assign(1, {nullptr, emptyCell});
    }

    template<typename ForwardIterator>
    HashMap(ForwardIterator start, ForwardIterator end) : HashMap() {
        while (start != end) {
            insert(*start);
            ++start;
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list) : HashMap() {
        for (auto &i: list) {
            insert(i);
            this->operator[](i.first) = i.second;
        }
    }

    explicit HashMap(const Hash &hasher) : hasher_(hasher) {
        arr_.assign(1, {nullptr, emptyCell});
    }

    template<typename ForwardIterator>
    HashMap(ForwardIterator start, ForwardIterator end, const Hash &hasher) : HashMap(hasher) {
        while (start != end) {
            insert(*start);
            ++start;
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list, const Hash &hasher) : HashMap(hasher) {
        for (auto &i: list) {
            insert(i);
        }
    }

    HashMap(const HashMap &other) {
        arr_.assign(other.arr_.size(), {nullptr, emptyCell});
        for (auto &i: other) {
            insert(i);
        }
    }

    HashMap(const HashMap &&other) noexcept: arr_(std::move(other.arr_)), size_(std::move(other.size_)),
                                             hasher_(std::move(other.hasher_)) {}

    iterator begin() {
        for (size_t i = 0; i < arr_.size(); ++i) {
            if (arr_[i].first != nullptr) {
                return iterator(arr_.data() + i, i, arr_.size());
            }
        }
        return end();
    }

    iterator end() {
        return iterator(arr_.data() + arr_.size(), arr_.size(), arr_.size());
    }

    const_iterator begin() const {
        for (size_t i = 0; i < arr_.size(); ++i) {
            if (arr_[i].first != nullptr) {
                return const_iterator(arr_.data() + i, i, arr_.size());
            }
        }
        return end();
    }

    const_iterator end() const {
        return const_iterator(arr_.data() + arr_.size(), arr_.size(), arr_.size());
    }

    [[nodiscard]] size_t size() const {
        return size_;
    }

    [[nodiscard]] bool empty() const {
        return size() == 0;
    }

    void insert(const std::pair<const KeyType, ValueType> &p) {
        if (find(p.first) != end()) {
            return;
        }
        size_++;
        if (size() > loadFactor * arr_.size()) {
            // increase
            std::vector<std::pair<HashMapItem *, size_t>> newArr(arr_.size() * changeFactor, {nullptr, emptyCell});
            for (size_t i = 0; i < arr_.size(); i++) {
                if (arr_[i].second != emptyCell) {
                    insertSomewhere(arr_[i].first, newArr);
                }
            }
            std::swap(arr_, newArr);
        }
        insertSomewhere(new HashMapItem(p.first, p.second), arr_);
    }

    void erase(const KeyType &key) {
        if (find(key) == end()) {
            return;
        }
        size_t index = hashCode(key, arr_.size());
        size_t it = 0;
        for (; it < arr_.size(); it++, index = (index + 1) % arr_.size()) {
            if (arr_[index].first->first == key) {
                delete arr_[index].first;
                arr_[index] = {nullptr, emptyCell};
                break;
            }
        }
        size_t pindex = index;
        it++, index = (index + 1) % arr_.size();
        for (; it < arr_.size(); it++, index = (index + 1) % arr_.size(), pindex = (pindex + 1) % arr_.size()) {
            if (arr_[index].second == 0 || arr_[index].second == emptyCell) {
                break;
            }
            std::swap(arr_[pindex], arr_[index]);
            arr_[pindex].second--;
        }

        size_--;
        if (size() < freeFactor * arr_.size()) {
            // decrease
            std::vector<std::pair<HashMapItem *, size_t>> newArr(arr_.size() / changeFactor, {nullptr, emptyCell});
            for (size_t i = 0; i < arr_.size(); i++) {
                if (arr_[i].second != emptyCell) {
                    insertSomewhere(arr_[i].first, newArr);
                }
            }
            std::swap(arr_, newArr);
        }
    }

    iterator find(const KeyType &key) {
        size_t index = hashCode(key, arr_.size());
        size_t psl = 0;
        for (size_t it = 0; it < arr_.size(); it++, index = (index + 1) % arr_.size()) {
            if (psl > arr_[index].second || arr_[index].second == emptyCell) {
                return end();
            }
            if (arr_[index].first->first == key) {
                return iterator(&arr_[index], index, arr_.size());
            }
            psl++;
        }
        return end();
    }

    const_iterator find(const KeyType &key) const {
        size_t index = hashCode(key, arr_.size());
        size_t psl = 0;
        for (size_t it = 0; it < arr_.size(); it++, index = (index + 1) % arr_.size()) {
            if (psl > arr_[index].second || arr_[index].second == emptyCell) {
                return end();
            }
            if (arr_[index].first->first == key) {
                return const_iterator(&arr_[index], index, arr_.size());
            }
            psl++;
        }
        return end();
    }

    ValueType &at(const KeyType &key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("no item");
        }
        return it->second;
    }

    const ValueType &at(const KeyType &key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("no item");
        }
        return it->second;
    }

    ValueType &operator[](const KeyType &k) {
        if (find(k) == end()) {
            insert({k, ValueType{}});
        }
        return at(k);
    }

    Hash hash_function() const {
        return hasher_;
    }

    void clear() {
        for (auto &i : arr_) {
            delete i.first;
        }
        arr_.assign(1, {nullptr, emptyCell});
        size_ = 0;
    }

    HashMap &operator=(const HashMap &other) {
        hasher_ = other.hasher_;
        if (arr_ == other.arr_) {
            assert(size() == other.size_);
            return *this;
        }
        for (auto &i: arr_) {
            delete i.first;
            i = {nullptr, emptyCell};
        }
        for (auto &i: other) {
            insert(i);
        }
        return *this;
    }

private:
    size_t hashCode(const KeyType &key, size_t cap) const {
        return hasher_(key) % cap;
    }

    void insertSomewhere(HashMapItem *it, std::vector<std::pair<HashMapItem *, size_t>> &to) {
        size_t index = hashCode(it->first, to.size());
        size_t psl = 0;
        while (true) {
            if (to[index].second == emptyCell) {
                to[index] = {it, psl};
                return;
            }
            if (psl > to[index].second) {
                std::swap(it, to[index].first);
                std::swap(psl, to[index].second);
            }
            psl++;
            index = (index + 1) % to.size();
        }
    }

    const long double freeFactor = 0.25;
    const long double loadFactor = 0.5;
    const int changeFactor = 2;
    const size_t emptyCell = SIZE_MAX;

    size_t size_{};
    std::vector<std::pair<HashMapItem *, size_t>> arr_;
    Hash hasher_;
};

#endif //ROBINHOODHASING_HASHMAP_H
