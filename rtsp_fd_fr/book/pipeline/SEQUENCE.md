# pipeline::Sequence

A pipeline sequence consists of a sequence of components.
The process method of pipeline seqeuence invokes process methods of each component in the given order.
The output of each but not the last component is passed to the succeeding component as its input.
The type of outputs and inputs doesn't have to be exactly the same.
The input of the succeeding compoenent can be a subsequence of the output of the preceding component if there is a valid and unambiguous way to select the data.

Here are some examples.<br>
```std::tuple<int, string> -> std::tuple<int>``` is valid. <br>
```std::tuple<int, string> -> std::tuple<int, double>``` is **not** valid. <br>
```std::tuple<string, string> -> std::tuple<string>``` is ambiguous and therefore invalid. <br>
```std::tuple<string, int, string> -> std::tuple<int, string>``` is valid because the selected string must be after the integer.

To create pipeline sequences, the global function ```pipeline::make_sequence``` takes a series of ```std::shared_ptr``` of components and returns a ```std::shared_ptr``` refering to the pipeline::Sequence created. With ```A``` and ```B```, two components classes defined, the following code creates a pipeline sequence. <br>
```pipeline::make_sequence(make_shared<A>(), make_shared<B>())``` <br>
The created pipeline::Sequence passes the input to its ```process``` method to ```A::process```, forwards the output of ```A::process``` to ```B::process```, and returns the output of ```B::process``` as its own output.

A pipeline::Sequence is not a derived class of pipeline::Component but can be deemed as a pipeline component.
It can be helpful in combining the components into a pipeline with switches and branches, which are coming up next. 
