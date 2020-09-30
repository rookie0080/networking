#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): cap(capacity) { 
    buf.resize(capacity);
}

size_t ByteStream::write(const string &data) {
    if (data.size() == 0)
        return 0;
    size_t byte_write = (data.size() > (cap - size)) ? cap - size : data.size(); 
    for (size_t i = 0; i < byte_write; i++) {
        buf[pos_for_wr] = data[i];
        pos_for_wr = (pos_for_wr + 1) % cap;
    }
    size += byte_write;
    total_write += byte_write;
    return byte_write;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string ret;
    size_t rd = pos_for_rd;
    for (size_t i = 0; i < len; i++) {
        ret = ret + buf[rd];
        rd = (rd + 1) % cap;
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    pos_for_rd = (pos_for_rd + len) % cap;
    size -= len;
    total_read += len;  
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t byte_read = (len > size) ? size : len;
    string ret = peek_output(byte_read);
    pop_output(byte_read);
    return ret;
}

void ByteStream::end_input() { is_eof = true; }

bool ByteStream::input_ended() const { return is_eof; }

size_t ByteStream::buffer_size() const { return size; }

bool ByteStream::buffer_empty() const { return (size == 0 ? true : false); }

bool ByteStream::eof() const { return is_eof && pos_for_wr == pos_for_rd ? true : false; }

size_t ByteStream::bytes_written() const { return total_write; }

size_t ByteStream::bytes_read() const { return total_read; }

size_t ByteStream::remaining_capacity() const { return cap - size; }
