#include "byte_stream.hh"
#include "fsm_stream_reassembler_harness.hh"
#include "stream_reassembler.hh"
#include "util.hh"

#include <exception>
#include <iostream>

using namespace std;

int main() {
    try {
        // 如果这个用例对了，那many用例的逻辑基本就不会错
        // {
        //     ReassemblerTestHarness test{65000};
        //     test.execute(SubmitSegment{"012", 20});
        //     test.execute(SubmitSegment{"901", 59});
        //     test.execute(SubmitSegment{"345", 63});
        //     test.execute(SubmitSegment{"678", 56});
        //     test.execute(SubmitSegment{"123", 31});
        //     test.execute(SubmitSegment{"678", 66});
        //     test.execute(SubmitSegment{"789", 37});
        //     test.execute(SubmitSegment{"2", 62});
        //     test.execute(SubmitSegment{"234", 72});
        //     test.execute(SubmitSegment{"45", 44});
        //     test.execute(SubmitSegment{"456", 4});
        //     test.execute(SubmitSegment{"3", 13});
        //     test.execute(SubmitSegment{"4567", 14});
        //     test.execute(SubmitSegment{"9012", 9});
        //     test.execute(SubmitSegment{"4", 34});
        //     test.execute(SubmitSegment{"2345", 52});
        //     test.execute(SubmitSegment{"78", 7});
        //     test.execute(SubmitSegment{"01", 70});
        //     test.execute(SubmitSegment{"6", 36});
        //     test.execute(SubmitSegment{"0123", 0});
        //     test.execute(SubmitSegment{"9", 69});
        //     test.execute(SubmitSegment{"6", 26});
        //     test.execute(SubmitSegment{"5", 35});
        //     test.execute(SubmitSegment{"7890", 27});
        //     test.execute(SubmitSegment{"345", 23});
        //     test.execute(SubmitSegment{"9", 19});
        //     test.execute(SubmitSegment{"8", 18});
        //     test.execute(SubmitSegment{"0123", 40});
        //     test.execute(SubmitSegment{"8901", 48});
        //     test.execute(SubmitSegment{"67", 46});

        //     test.execute(BytesAssembled(75));
        //     test.execute(UnassembledBytes{0});
        // }

        {
            ReassemblerTestHarness test{65000};

            test.execute(BytesAssembled(0));
            test.execute(BytesAvailable(""));
            test.execute(NotAtEof{});
        }

        {
            ReassemblerTestHarness test{65000};

            test.execute(SubmitSegment{"a", 0});

            test.execute(BytesAssembled(1));
            test.execute(BytesAvailable("a"));
            test.execute(NotAtEof{});
        }

        {
            ReassemblerTestHarness test{65000};

            test.execute(SubmitSegment{"a", 0}.with_eof(true));

            test.execute(BytesAssembled(1));
            test.execute(BytesAvailable("a"));
            test.execute(AtEof{});
        }

        {
            ReassemblerTestHarness test{65000};

            test.execute(SubmitSegment{"", 0}.with_eof(true));

            test.execute(BytesAssembled(0));
            test.execute(BytesAvailable(""));
            test.execute(AtEof{});
        }

        {
            ReassemblerTestHarness test{65000};

            test.execute(SubmitSegment{"b", 0}.with_eof(true));

            test.execute(BytesAssembled(1));
            test.execute(BytesAvailable("b"));
            test.execute(AtEof{});
        }

        {
            ReassemblerTestHarness test{65000};

            test.execute(SubmitSegment{"", 0});

            test.execute(BytesAssembled(0));
            test.execute(BytesAvailable(""));
            test.execute(NotAtEof{});
        }

        {
            ReassemblerTestHarness test{8};

            test.execute(SubmitSegment{"abcdefgh", 0});

            test.execute(BytesAssembled(8));
            test.execute(BytesAvailable{"abcdefgh"});
            test.execute(NotAtEof{});
        }

        {
            ReassemblerTestHarness test{8};

            test.execute(SubmitSegment{"abcdefgh", 0}.with_eof(true));

            test.execute(BytesAssembled(8));
            test.execute(BytesAvailable{"abcdefgh"});
            test.execute(AtEof{});
        }

        {
            ReassemblerTestHarness test{8};

            test.execute(SubmitSegment{"abc", 0});
            test.execute(BytesAssembled(3));

            test.execute(SubmitSegment{"bcdefgh", 1}.with_eof(true));

            test.execute(BytesAssembled(8));
            test.execute(BytesAvailable{"abcdefgh"});
            test.execute(AtEof{});
        }

        {
            ReassemblerTestHarness test{8};

            test.execute(SubmitSegment{"abc", 0});
            test.execute(BytesAssembled(3));
            test.execute(NotAtEof{});

            test.execute(SubmitSegment{"ghX", 6}.with_eof(true));
            test.execute(BytesAssembled(3));
            test.execute(NotAtEof{});

            test.execute(SubmitSegment{"cdefg", 2});
            test.execute(BytesAssembled(8));
            test.execute(BytesAvailable{"abcdefgh"});
            test.execute(NotAtEof{});
        }
    } catch (const exception &e) {
        cerr << "Exception: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
