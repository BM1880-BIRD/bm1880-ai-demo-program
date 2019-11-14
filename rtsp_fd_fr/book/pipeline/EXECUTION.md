# Pipeline Execution

```pipeline::make``` takes a series of components and create a pipeline object wrapping the components.
A pipeline object has a function ```execute_once``` which puts an empty data entry into the pipeline.
The process will go through the pipeline and execute it.
If a runtime_error is thrown during the execution, execute_once will return false; otherwise, it returns true.

The first component usually serves as the data source which takes an empty data as a trigger.
It produces a data entry for each trigger to be the input of succeeding components.
The last component usually takes the input data and write the data to somewhere outside of the pipeline.
It simply returns empty data and ends the iteration, allowing the execution to enter the next iteration.
The functions ```pipeline::wrap_source``` and ```pipeline::wrap_sink``` serves the purpose by wrapping DataSource and DataSink into pipeline components.
