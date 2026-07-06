#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace libsys {

// Singleflight: 防缓存击穿 (Cache Stampede)
// 当多个并发请求查询同一个未命中缓存的 key 时, 仅允许一个请求执行
// factory 回源 (PG), 其余请求在 mutex 上阻塞等待, 命中后共享同一份结果.
template <typename T>
class Singleflight {
public:
    using Factory = std::function<T()>;

    static T do(const std::string &key, Factory factory) {
        std::shared_ptr<Call> call;
        {
            std::lock_guard<std::mutex> lk(mapMutex());
            auto it = calls().find(key);
            if (it != calls().end()) {
                call = it->second;
            } else {
                call = std::make_shared<Call>();
                calls()[key] = call;
            }
        }
        std::unique_lock<std::mutex> lk(call->mtx);
        if (call->done) {
            return call->value;
        }
        call->value = factory();
        call->done = true;
        T v = call->value;
        lk.unlock();
        {
            std::lock_guard<std::mutex> mlk(mapMutex());
            calls().erase(key);
        }
        return v;
    }

private:
    struct Call {
        std::mutex mtx;
        bool done{false};
        T value;
    };
    static std::mutex &mapMutex() {
        static std::mutex m;
        return m;
    }
    static std::unordered_map<std::string, std::shared_ptr<Call>> &calls() {
        static std::unordered_map<std::string, std::shared_ptr<Call>> c;
        return c;
    }
};

}  // namespace libsys
