#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timeout(retx_timeout) {}

// #include <iostream>
uint64_t TCPSender::bytes_in_flight() const {
    // cout << _next_seqno - _send_base << " " << _send_base << ' ' << _next_seqno << endl;
    return _next_seqno - _send_base;
}

void TCPSender::fill_window() {
    // NOTE: fill_window could be invoked in certain situations even if fin has been acknowledged
    if (_window_size == 0 || _fin_flag || (_syn_flag && _stream.buffer_empty() && !_stream.input_ended())) 
        return;

    if (_syn_flag == false) {
        // send a syn seg
        _syn_flag = true;
        TCPSegment seg;
        seg.header().syn = true;
        seg.header().seqno = wrap(0, _isn);
        _next_seqno = 1;
        _window_size--;
        _segments_out.push(seg);
        _copy_segments_out.push(seg);
    } else if (_stream.eof()) {
        // send a fin seg without payload
        TCPSegment seg;
        seg.header().fin = true;
        seg.header().seqno = wrap(_next_seqno, _isn);
        _next_seqno++;     // fin also need consider window size
        _window_size--;
        _fin_flag = true;
        _segments_out.push(seg);
        _copy_segments_out.push(seg);
    } else {
        // send as much segments as possible
        while (!_stream.buffer_empty() && _window_size > 0) { 
            TCPSegment seg;
            seg.header().seqno = wrap(_next_seqno, _isn);
            uint64_t len_new_seg = (TCPConfig::MAX_PAYLOAD_SIZE < _window_size) ? TCPConfig::MAX_PAYLOAD_SIZE : _window_size;
            seg.payload() = _stream.read(len_new_seg);
            _window_size -= seg.length_in_sequence_space();
            if (_window_size > 0 && _stream.eof()) { // NOTE: fin segment can carry data!
                seg.header().fin = true;
                _fin_flag = true;
                _window_size--;
            }
            _next_seqno += seg.length_in_sequence_space(); 
            _segments_out.push(seg);
            _copy_segments_out.push(seg);
            if (_fin_flag) break;
        }
    }
    if (!_is_timer_running) {
        _is_timer_running = true;
        _total_time = 0;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t ackno_64 = unwrap(ackno, _isn, _send_base);
    if (_send_base == 0 && ackno_64 == 1) { // ack for syn
        _send_base = 1;
        _total_time = 0;
        _copy_segments_out.pop();
        _retransmission_timeout = _initial_retransmission_timeout; // NOTE
        _total_time = 0;
        _consecutive_retransmissions = 0; 
    } else if (_fin_flag && _copy_segments_out.size() == 1 && ackno_64 == _next_seqno) {
        _send_base += _copy_segments_out.front().length_in_sequence_space(); // length contain fin!!
        _copy_segments_out.pop();
    } else if (!_copy_segments_out.empty() && ackno_64 >= _send_base + _copy_segments_out.front().length_in_sequence_space()) {
        uint64_t copy_seg_seqno = unwrap(_copy_segments_out.front().header().seqno, _isn, _send_base);
        uint64_t copy_seg_len = _copy_segments_out.front().length_in_sequence_space();
        while (copy_seg_seqno + copy_seg_len <= ackno_64) {
            _send_base += _copy_segments_out.front().length_in_sequence_space();
            _copy_segments_out.pop();
            if (_copy_segments_out.empty()) break;
            copy_seg_seqno = unwrap(_copy_segments_out.front().header().seqno, _isn, _send_base);
            copy_seg_len = _copy_segments_out.front().length_in_sequence_space();
        }
        _retransmission_timeout = _initial_retransmission_timeout;
        _total_time = 0;
        _consecutive_retransmissions = 0;
        
    } 
    
    if (bytes_in_flight() == 0) {  // stop the timer if all outstanding seg have been acked
        _is_timer_running = false;
    } else if (bytes_in_flight() >= window_size) { // note: do not send FIN if this would make the segment exceed the window size
        _window_size = 0;
        return;
    }
    
    if (window_size == 0) {
        _window_size = 1;
        _win_size_zero = true;
    } else {
        _window_size = window_size;
        _win_size_zero = false;
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _total_time += ms_since_last_tick;
    if (!_copy_segments_out.empty() && _is_timer_running && _total_time >= _retransmission_timeout) {
        _segments_out.push(_copy_segments_out.front());
        _consecutive_retransmissions++;
        if (!_win_size_zero)  // NOTE: do not double the value if window size is non zero!
            _retransmission_timeout *= 2;
        _total_time = 0;
    } 
}

unsigned int TCPSender::consecutive_retransmissions() const { 
    return _consecutive_retransmissions;
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    seg.payload() = {};
    _segments_out.push(seg);
}
