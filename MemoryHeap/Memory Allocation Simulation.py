import random

class Block:
    def __init__(self, start, size, free=True):
        self.start = start
        self.size = size
        self.free = free
        self.prev = None
        self.next = None

class MemoryAllocator:
    def __init__(self, total_memory):
        self.total_memory = total_memory
        self.head = Block(0, total_memory)

    def first_fit(self, size):
        current = self.head
        while current:
            if current.free and current.size >= size:
                return self._allocate_block(current, size)
            current = current.next
        print(f"No suitable block for size {size}")
        return None

    def _allocate_block(self, block, size):
        if block.size == size:
            block.free = False
            return block
        new_block = Block(block.start + size, block.size - size)
        new_block.next = block.next
        new_block.prev = block
        if block.next:
            block.next.prev = new_block
        block.next = new_block
        block.size = size
        block.free = False
        return block

    def free_block(self, block):
        block.free = True
        if block.next and block.next.free:
            block.size += block.next.size
            block.next = block.next.next
            if block.next:
                block.next.prev = block
        if block.prev and block.prev.free:
            block.prev.size += block.size
            block.prev.next = block.next
            if block.next:
                block.next.prev = block.prev

    def print_memory(self):
        current = self.head
        print("Memory:")
        while current:
            status = "Free" if current.free else "Used"
            print(f"[{current.start}-{current.start + current.size - 1}] Size:{current.size} {status}")
            current = current.next
        print("-" * 40)

# Simulation
allocator = MemoryAllocator(1000)
allocated = []
alloc_attempts = 0
free_count = 0

while alloc_attempts < 50 or free_count < 30:
    action = random.choice(['alloc', 'free'])

    if action == 'alloc' and alloc_attempts < 50:
        size = random.randint(5, 50)
        block = allocator.first_fit(size)
        alloc_attempts += 1
        if block:
            allocated.append(block)
        print(f"Alloc attempt {alloc_attempts}: size {size}")
        allocator.print_memory()

    if action == 'free' and free_count < 30 and allocated:
        block = random.choice(allocated)
        allocator.free_block(block)
        allocated.remove(block)
        free_count += 1
        print(f"Free {free_count}: block starting at {block.start}")
        allocator.print_memory()

print("Final Memory State:")
allocator.print_memory()
