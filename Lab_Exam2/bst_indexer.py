"""
bst_indexer.py

Indexes words in a text file using a Binary Search Tree (BST).

Usage:
    python bst_indexer.py <input_file> <index_output_file> [optional_load_bst] [optional_save_bst]

See the bottom of this file for a testing example.
"""
import sys
import re
from typing 
import Optional, Iterator, List

# --- Queue implementation for line numbers ---
class QueueNode:
    """Node for singly-linked list queue of line numbers."""
    def __init__(self, value: int):
        self.value = value
        self.next: Optional['QueueNode'] = None

class Queue:
    """Queue of line numbers (singly-linked list)."""
    def __init__(self):
        self.head: Optional[QueueNode] = None
        self.tail: Optional[QueueNode] = None

    def append(self, value: int):
        """Append a value to the queue if not duplicate of last."""
        if self.tail and self.tail.value == value:
            return
        node = QueueNode(value)
        if not self.head:
            self.head = self.tail = node
        else:
            assert self.tail is not None
            self.tail.next = node
            self.tail = node

    def __iter__(self) -> Iterator[int]:
        current = self.head
        while current:
            yield current.value
            current = current.next

    def to_list(self) -> List[int]:
        return list(self)

# --- BST implementation ---
class BSTNode:
    """Node of the BST: stores word, queue of line numbers, left/right children."""
    def __init__(self, word: str):
        self.word = word
        self.lines = Queue()
        self.left: Optional['BSTNode'] = None
        self.right: Optional['BSTNode'] = None

class BST:
    """Binary Search Tree for word indexing."""
    def __init__(self):
        self.root: Optional[BSTNode] = None

    def insert(self, word: str, line: int):
        """Insert word with line number. If exists, append line if not duplicate."""
        def _insert(node: Optional[BSTNode], word: str, line: int) -> BSTNode:
            if node is None:
                node = BSTNode(word)
                node.lines.append(line)
                return node
            if word == node.word:
                node.lines.append(line)
            elif word < node.word:
                node.left = _insert(node.left, word, line)
            else:
                node.right = _insert(node.right, word, line)
            return node
        self.root = _insert(self.root, word, line)

    def search(self, word: str) -> Optional[BSTNode]:
        """Search for a word in the BST. Return node or None."""
        node = self.root
        while node:
            if word == node.word:
                return node
            elif word < node.word:
                node = node.left
            else:
                node = node.right
        return None

    def inorder_traverse(self) -> Iterator[tuple[str, List[int]]]:
        """Yield (word, [lines]) in ASCII order."""
        def _inorder(node: Optional[BSTNode]):
            if node:
                yield from _inorder(node.left)
                yield (node.word, node.lines.to_list())
                yield from _inorder(node.right)
        return _inorder(self.root)

    def save_to_file(self, path: str):
        """Serialize BST to file in in-order traversal."""
        with open(path, 'w', encoding='utf-8') as f:
            for word, lines in self.inorder_traverse():
                line_str = ','.join(str(num) for num in lines)
                f.write(f"{word}\t{line_str}\n")

    def load_from_file(self, path: str):
        """Load BST from file. Skips duplicate line entries for a word."""
        with open(path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.rstrip('\n')
                if not line:
                    continue
                if '\t' not in line:
                    continue
                word, lines_str = line.split('\t', 1)
                seen = set()
                for num_str in lines_str.split(','):
                    num_str = num_str.strip()
                    if not num_str:
                        continue
                    num = int(num_str)
                    if num not in seen:
                        self.insert(word, num)
                        seen.add(num)

# --- Main program logic ---
def print_usage():
    print("Usage:")
    print("  python bst_indexer.py <input_file> <index_output_file> [optional_load_bst] [optional_save_bst]")
    print("    <input_file>: text file to scan (required)")
    print("    <index_output_file>: file to write the final alphabetical index (required)")
    print("    [optional_load_bst]: (optional) previously saved BST file to load")
    print("    [optional_save_bst]: (optional) file to save the final BST after processing")


def main():
    if len(sys.argv) < 3:
        print_usage()
        sys.exit(1)
    input_file = sys.argv[1]
    index_output_file = sys.argv[2]
    optional_load_bst = sys.argv[3] if len(sys.argv) > 3 else None
    optional_save_bst = sys.argv[4] if len(sys.argv) > 4 else None

    bst = BST()
    # Load BST if provided
    if optional_load_bst:
        try:
            bst.load_from_file(optional_load_bst)
        except Exception as e:
            print(f"Error loading BST from '{optional_load_bst}': {e}")
            sys.exit(1)

    # Parse input file and index words
    word_splitter = re.compile(r'[ \t\n,;:.]+')
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                words = word_splitter.split(line.rstrip('\n'))
                seen_words = set()
                for word in words:
                    if word == '':
                        continue
                    if word not in seen_words:
                        bst.insert(word, line_num)
                        seen_words.add(word)
    except Exception as e:
        print(f"Error reading input file '{input_file}': {e}")
        sys.exit(1)

    # Write index output file
    try:
        with open(index_output_file, 'w', encoding='utf-8') as f:
            for word, lines in bst.inorder_traverse():
                line_str = ', '.join(str(num) for num in lines)
                f.write(f"{word}: {line_str}\n")
    except Exception as e:
        print(f"Error writing index output file '{index_output_file}': {e}")
        sys.exit(1)

    # Save BST if requested
    if optional_save_bst:
        try:
            bst.save_to_file(optional_save_bst)
        except Exception as e:
            print(f"Error saving BST to '{optional_save_bst}': {e}")
            sys.exit(1)

if __name__ == "__main__":
    main()

"""
# --- Testing Example ---
# Suppose input.txt contains:
#   Cat sat on the mat.
#   The cat chased a rat.
#   A rat ran.
#
# Command:
#   python bst_indexer.py input.txt output.txt
#
# Expected output.txt:
#   A: 3
#   Cat: 1
#   The: 2
#   a: 2
#   cat: 2
#   chased: 2
#   mat: 1
#   on: 1
#   rat: 2, 3
#   ran: 3
#   sat: 1
#   the: 1
"""
