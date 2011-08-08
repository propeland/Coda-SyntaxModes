package bytes

// Simple byte buffer for marshaling data.

import (
	"io"
	"os"
	"utf8"
)

// A Buffer is a variable-sized buffer of bytes with Read and Write methods.
// The zero value for Buffer is an empty buffer ready to use.
type Buffer struct {
	buf       []byte            // contents are the bytes buf[off : len(buf)]
	off       int               // read at &buf[off], write at &buf[len(buf)]
	runeBytes [utf8.UTFMax]byte // avoid allocation of slice on each WriteByte or Rune
	bootstrap [64]byte          // memory to hold first slice; helps small buffers (Printf) avoid allocation.
	lastRead  readOp            // last read operation, so that Unread* can work correctly.
}

// The readOp constants describe the last action performed on
// the buffer, so that UnreadRune and UnreadByte can
// check for invalid usage.
type readOp int

const (
	opInvalid  readOp = iota // Non-read operation.
	opReadRune               // Read rune.
	opRead                   // Any other read operation.
)

// Bytes returns a slice of the contents of the unread portion of the buffer;
// len(b.Bytes()) == b.Len().  If the caller changes the contents of the
// returned slice, the contents of the buffer will change provided there
// are no intervening method calls on the Buffer.
func (b *Buffer) Bytes() []byte { return b.buf[b.off:] }

// String returns the contents of the unread portion of the buffer
// as a string.  If the Buffer is a nil pointer, it returns "<nil>".
func (b *Buffer) String() string {
	if b == nil {
		// Special case, useful in debugging.
		return "<nil>"
	}
	return string(b.buf[b.off:])
}
