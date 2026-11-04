// code/movesavebounds.h
//
// Range checks for the path counters a saved game restores into fixed arrays
// (MechWarrior::copyFromData). Pure logic, so the unit tests can reach it.

#ifndef MOVESAVEBOUNDS_H
#define MOVESAVEBOUNDS_H

// count entries of an array of capacity, with cur the entry being worked on.
// cur may equal count (finished), and may be left over from an earlier, longer
// path while count has been reset (calcMovePath clears only the count), but it
// must never index past the array.
inline bool savedStepsValid(long count, long cur, long capacity)
{
	if (count < 0 || count > capacity || cur < 0)
		return false;
	return cur < capacity || cur == count;
}

// a count with no "current" entry
inline bool savedCountValid(long count, long capacity)
{
	return count >= 0 && count <= capacity;
}

// an index into an array of capacity, or -1 for none
inline bool savedIndexValid(long index, long capacity)
{
	return index >= -1 && index < capacity;
}

#endif
