#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}
using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader header = seg.header();
    WrappingInt32 seqno(header.seqno);
    if (header.syn) {
        _isn = WrappingInt32(header.seqno); 
        _isn_set = true;
        seqno = WrappingInt32(header.seqno) + 1;
    }
    size_t checkpoint = _reassembler.stream_out().bytes_written();
    size_t abs_seqno_64 = unwrap(seqno, _isn, checkpoint); // first unassembled
    size_t stream_index = abs_seqno_64 - 1;
    string data = seg.payload().copy();
    _reassembler.push_substring(data, stream_index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn_set) return {};
    if (_reassembler.stream_out().input_ended()) { // 如果Reassembler已经组装完毕，返回fin之后的那个序号
        return WrappingInt32(wrap(_reassembler.stream_out().bytes_written() + 1, _isn)) + 1;
    }
    // first_unassembled是stream的索引，要先转换成64-bit absolute seqno（简单加1），再包装成32-bit seqno
    return WrappingInt32(wrap(_reassembler.stream_out().bytes_written() + 1, _isn)); 
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.stream_out().bytes_read() + _capacity - _reassembler.stream_out().bytes_written();
}

