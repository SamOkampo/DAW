#pragma once

#include "flowdaw/Types.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace flowdaw {

// UI/control-thread-only Arrangement selection state. Selection is keyed by
// persistent block IDs instead of vector indices so unrelated edits/reordering
// cannot silently retarget the selection. Storage is fixed-size so selection
// changes never require heap allocation and pathological selections stay bounded.
class ArrangementSelection {
public:
    enum class Kind { None, AudioClip, PatternClip };

    struct Item {
        Kind kind = Kind::None;
        Id id = 0;
        Id trackId = 0;

        [[nodiscard]] bool valid() const noexcept {
            return kind != Kind::None && id != 0 && trackId != 0;
        }

        friend bool operator==(const Item&, const Item&) = default;
    };

    static constexpr std::size_t kMaxItems = 256;

    [[nodiscard]] const Item& item() const noexcept { return primary_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool contains(Kind kind, Id trackId, Id id) const noexcept {
        return contains(Item{kind, id, trackId});
    }

    void selectAudio(Id trackId, Id clipId) noexcept {
        selectOnly({Kind::AudioClip, clipId, trackId});
    }

    void selectPattern(Id trackId, Id placementId) noexcept {
        selectOnly({Kind::PatternClip, placementId, trackId});
    }

    // Adds a valid item while preserving the existing primary selection. Returns
    // false for invalid/duplicate items or when the bounded selection is full.
    bool add(Kind kind, Id trackId, Id id) noexcept {
        const Item next{kind, id, trackId};
        if (!next.valid() || contains(next) || size_ >= kMaxItems)
            return false;
        items_[size_++] = next;
        if (!primary_.valid())
            primary_ = next;
        return true;
    }

    bool toggle(Kind kind, Id trackId, Id id) noexcept {
        const Item target{kind, id, trackId};
        if (!target.valid())
            return false;
        for (std::size_t i = 0; i < size_; ++i) {
            if (items_[i] != target)
                continue;
            std::move(items_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                      items_.begin() + static_cast<std::ptrdiff_t>(size_),
                      items_.begin() + static_cast<std::ptrdiff_t>(i));
            --size_;
            items_[size_] = {};
            primary_ = size_ == 0 ? Item{} : items_[0];
            return true;
        }
        return add(kind, trackId, id);
    }

    void clear() noexcept {
        std::fill(items_.begin(), items_.begin() + static_cast<std::ptrdiff_t>(size_), Item{});
        size_ = 0;
        primary_ = {};
    }

private:
    [[nodiscard]] bool contains(const Item& target) const noexcept {
        return std::find(items_.begin(), items_.begin() + static_cast<std::ptrdiff_t>(size_), target)
            != items_.begin() + static_cast<std::ptrdiff_t>(size_);
    }

    void selectOnly(Item next) noexcept {
        clear();
        if (!next.valid())
            return;
        items_[0] = next;
        size_ = 1;
        primary_ = next;
    }

    std::array<Item, kMaxItems> items_{};
    std::size_t size_ = 0;
    Item primary_;
};

} // namespace flowdaw
