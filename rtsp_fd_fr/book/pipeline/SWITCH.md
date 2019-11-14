# pipeline::Switch

A pipeline switch consists of a set of components, each components is associated with a string as its tag.
The first element in the input type of ```pipeline::Switch::process``` must be a string.
The switch takes the string and matches it with the tags of those components.
When there is a match, process function of the corressponding component is invoked with the input data.
Optionally, a default component can be set and will be invoked when there is no tag matching the input string.

To create pipeline switches, the global function ```pipeline::make_switch``` takes a series of ```std::pair<std::string, std::shared_ptr<T>>```
where ```T``` is the type of the component. With ```A``` and ```B```, two component classes defined, the following code creates a pipe switch
that invokes ```A::process``` when tag equals "a" and ```B::process``` when tag equals "b". <br>
```
pipeline::make_switch(
	make_pair("a", make_shared<A>()),
	make_pair("b", make_shared<B>())
)```
In addition, to set default behavior of the switch, append a pair at the end of parameters with a default constructed ```pipeline::switch_default``` as the tag.
```
pipeline::make_switch(
	make_pair("a", make_shared<A>()),
	make_pair("b", make_shared<B>()),
	make_pair(pipeline::switch_default(), make_shared<D>())
)```

