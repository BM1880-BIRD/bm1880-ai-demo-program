# pipeline::Branch

A pipeline branch consists of two components.
```pipeline::Branch::process``` calls both components and concatenates the output sequences in the order constructing the branch.

To create a pipeline::Branch, one can use the global function ```pipeline::make_branch```.
It takes two shared_ptrs to components and returns a shared pointer refering to the branch.

