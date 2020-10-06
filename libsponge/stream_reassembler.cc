#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}
#include <iostream>  // for debugging
using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity): 
    _output(capacity),
    _capacity(capacity),
    _bytes_unassembled(0),
    _substr_waiting({}),
    _flag_eof(false),
    _pos_eof(0) {}

void StreamReassembler::insert_substr_waiting(const struct Node &node){
    if (_substr_waiting.empty()) {
        _substr_waiting.insert(node);
        _bytes_unassembled += node.data.size();
        return;
    }

    struct Node tmp = node;
    auto it = _substr_waiting.lower_bound(node);   // lower_bound返回不小于目标值的第一个对象的迭代器
    size_t x = tmp.index, sz = tmp.data.size();

    if (it != _substr_waiting.begin()) { // node的左边有节点，
        it--;
        if (x < it->index + it->data.size() ) {     // node与左边相交（不相邻）或被包含
            if (x + sz <= it->index + it->data.size())
                return; // 被左边包含
            tmp.data = it->data + tmp.data.substr(it->index + it->data.size() - x);
            tmp.index = it->index;
            x = tmp.index; sz = tmp.data.size();
            _bytes_unassembled -= it->data.size();
            _substr_waiting.erase(it++);
        } else 
            it++;
    } 
    // 考察是否能与右边的节点合并
    while (it != _substr_waiting.end() && x + sz > it->index) {
        if (x >= it->index && x + sz < it->index + it->data.size())
            return; // 被右边包含
        if (x + sz < it->index + it->data.size()) { // 与右边相交
            tmp.data += it->data.substr(x + sz - it->index);
        }         
        _bytes_unassembled -= it->data.size();   // 包含有右边或仅为相交                             
        _substr_waiting.erase(it++);   
    }
    _substr_waiting.insert(tmp);
    _bytes_unassembled += tmp.data.size();
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    struct Node node{data, index};
    size_t first_unread = _output.bytes_read();
    size_t first_unassembled = _output.bytes_written();
    size_t first_unaccept = first_unread + _capacity;

    if (index + data.size() < first_unassembled || index >= first_unaccept)  // 超出这个范围直接不作处理
        return;
    if (index + data.size() > first_unaccept)
        node.data = node.data.substr(0, first_unaccept - index); // 如果超出了capacity，把需要处理的部分截下来

    if (index <= first_unassembled) {
        _output.write(node.data.substr(first_unassembled - index));
    } else {
        insert_substr_waiting(node);
    }
    // 写入
    auto it = _substr_waiting.begin();
    while (it->index <= _output.bytes_written()) {
        if (it->index + it->data.size() > node.index + node.data.size()) // 被包含就不用写入了
            _output.write(it->data.substr(_output.bytes_written() - it->index));
        _bytes_unassembled -= it->data.size();
        _substr_waiting.erase(it++);
    }

    if (eof) {
        _flag_eof = true;
        _pos_eof = index + data.size(); 
    }
    if (_flag_eof && _output.bytes_written() == _pos_eof)
        _output.end_input();  
    // cout << ", bytes_written: " << _output.bytes_written() << ", bytes_unassembled: " << _bytes_unassembled << endl;  
}

size_t StreamReassembler::unassembled_bytes() const { return _bytes_unassembled; }

bool StreamReassembler::empty() const { return _bytes_unassembled == 0; }
