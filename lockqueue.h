

/**
 * @name  freelock queue
 * @brief 
 * 
 */
#ifndef _LOCK_QUEUE_H
#define _LOCK_QUEUE_H
#include <atomic>

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

public:
    LockQueue() : head(nullptr), tail(nullptr) {}

    void enqueue(const T& value) {
        Node* newNode = new Node(value);
        newNode->next.store(nullptr, std::memory_order_relaxed);

        Node* prevTail = tail.exchange(newNode, std::memory_order_acq_rel);
        prevTail->next.store(newNode, std::memory_order_release);
    }

    bool dequeue(T& result) {
        Node* oldHead = head.load(std::memory_order_acquire);
        Node* newHead = oldHead->next.load(std::memory_order_relaxed);

        if (newHead == nullptr) {
            return false; // Queue is empty
        }

        result = newHead->data;
        head.store(newHead, std::memory_order_release);
        delete oldHead;

        return true;
    }
};

#endif // !_LOCK_QUEUE_H
