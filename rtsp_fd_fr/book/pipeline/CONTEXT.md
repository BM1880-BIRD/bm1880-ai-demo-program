# pipeline::Context

A pipeline::Context is a object that enables data sharing among components in a single pipeline or multiple pipelines.
It is a key-value mapping where each value has an individual type.
The type is defined by the first call to set or set_default method.
Setting or getting values with mismatching types will return false or null pointer as an error.

To enable components to load from the context, one can call set_context method of the pipeline object, which then
sets the context to all component in the pipeline.
One can also set the same context among multiple pipeline objects to allow data sharing among pipelines.
