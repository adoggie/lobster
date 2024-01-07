

/**
 * @name  freelock queue
 * @brief 
 * 
 */
#ifndef _LOCK_QUEUE_H
#define _LOCK_QUEUE_H
#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <list>
#include <boost/lockfree/spsc_queue.hpp>

template<typename T>
class LockQueue {
private:
    struct Node {
        T data;
        std::atomic<Node*> next;

        Node(const T& value) : data(value), next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;
    std::shared_mutex   mtx;
    std::list<T>    list_;
//    boost::lockfree::spsc_queue< int, boost::lockfree::capacity< 1024 > > spsc_queue;
    boost::lockfree::spsc_queue< T,boost::lockfree::capacity< 1024 > > spsc_queue;
public:
    LockQueue() : head(nullptr), tail(nullptr) {}

    void enqueue(const T& value) {
        std::unique_lock<std::shared_mutex> lock(mtx);
        list_.push_back(value);
    }

    void _enqueue(const T& value) {
        Node* newNode = new Node(value);
        newNode->next.store(nullptr, std::memory_order_relaxed);

        Node* prevTail = tail.load(std::memory_order_acquire);
        if (prevTail == nullptr) {
            // The queue is currently empty, so both head and tail should be updated.
            head.store(newNode, std::memory_order_release);
            tail.store(newNode, std::memory_order_release);
        } else {
            // The queue is not empty, so we only need to update the tail.
            prevTail->next.store(newNode, std::memory_order_release);
            tail.store(newNode, std::memory_order_release);
        }
    }

    bool dequeue(T& result){
        std::shared_lock<std::shared_mutex> lock(mtx);
        if( list_.size() == 0 ){
            return false;
        }
        result = list_.back();
        list_.pop_back();
        return true;
    }

    bool _dequeue(T& result) {
        Node* oldHead = head.load(std::memory_order_acquire);
        if (oldHead == nullptr) {
            return false; // Queue is empty
        }

        Node* newHead = oldHead->next.load(std::memory_order_relaxed);

        if (newHead == nullptr) {
            // There is only one element in the queue and it's being dequeued.
            tail.store(nullptr, std::memory_order_release);
            if (head.compare_exchange_strong(oldHead, newHead, std::memory_order_acq_rel)) {
                result = oldHead->data;
                delete oldHead;
                return true;
            } else {
                return false;
            }
        }

        result = oldHead->data;
        head.store(newHead, std::memory_order_release);
        delete oldHead;

        return true;
    }

};

#endif // !_LOCK_QUEUE_H
