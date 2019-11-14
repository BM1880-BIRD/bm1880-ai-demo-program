# pipeline::Component

A pipeline component defines a sub-routine that processes input data and produces output data.
Each component can either be stateful or state-less depending on the implementation.

To define a pipeline component, one can implement a class inheriting <br>
```pipeline::Component<std::tuple<...>>``` <br>
Where the first ellipsis consists of input types.
The class must also implement a member function <br>
```std::tuple<...> process(std::tuple<...> &&)``` <br>
specifying how the input data is processed.

The following is a component that take two integers and output the sum. <br>
```
class Sum : public pipeline::Component<std::tuple<int, int>> {
public:
	std::tuple<int> process(std::tuple<int, int> &&input) {
		return std::get<0>(input) + std::get<1>(input);
	}
};
```
