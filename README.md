<!-- markdownlint-disable MD041 -->
# Biosignals extension for DuckDB

**NOTE**: This extension is not supported in WebAssembly.

## Features

* Import EDF(+)/BDF(+) data and metadata directly into DuckDB.

* Support for cloud files through the DuckDB [`httpfs`](https://duckdb.org/docs/current/core_extensions/httpfs/overview)
extension.

| **Format**   | **Support** |
|--------------|---------|
| EDF, BDF     | Yes     |
| EDF+C, BDF+C | Yes     |
| EDF+D, BDF+D | Yes     |

## Quick start

```sql
INSTALL biosignals FROM community;
LOAD biosignals;

FROM read_edf_global_header('path/to/edf_file.edf');
FROM read_edf_global_header('path/to/many/edf/files/*.edf');
```

Import data from S3:

```sql
LOAD httpfs;

CREATE OR REPLACE SECRET my_secret (
    TYPE s3,
    PROVIDER config,
    KEY_ID 'my_key',
    SECRET 'my_secret_key',
    URL_STYLE 'path'
);

FROM read_edf_global_header('s3://my-bucket/path/to/my_edf_file.edf');
FROM read_edf_global_header('s3://my-bucket/path/to/*.edf');
```

## Table functions

All included table functions take as input the file path or glob pattern of the EDF(+)/BDF(+) file to be imported. Glob
patterns are supported both local and remote files.

### `read_edf_global_header`

Import the global header information into a table, one row per file.

**Return schema:**

| Column Name | Data Type | Description |
|-------------|-----------|-------------|
| `filename` | `VARCHAR` | Path to the EDF(+)/BDF(+) file. |
| `format` | `VARCHAR` | The file format. Options are: EDF, BDF, EDF+C, BDF+C, EDF+D, BDF+D. |
| `version` | `VARCHAR` | The version included in the header. Null if empty. |
| `patient_id` | `VARCHAR` | The patient ID included in the header. Null if empty. |
| `recording_id` | `VARCHAR` | The recording ID included in the header. Null if empty. |
| `start_datetime` | `TIMESTAMP` | The starting date and time of the recording, not including timezone information. |
| `num_records` | `INTEGER` | The number of records included in the file. |
| `duration` | `FLOAT` | The duration of each record in seconds. |
| `num_signals` | `INTEGER` | The number of signals in the recording. |

### `read_edf_signal_headers`

Import the header information for each individual signals into a table, one row per every file signal.

**Return schema:**

| Column Name | Data Type | Description |
|-------------|-----------|-------------|
| `filename` | `VARCHAR` | Path to the EDF(+)/BDF(+) file. |
| `signal_id` | `UINTEGER` | The signal ID, starting from 1 in the order of appearance in the header. |
| `label` | `VARCHAR` | The signal label. Null if empty. |
| `transducer_type` | `VARCHAR` | The transducer type. Null if empty. |
| `physical_dim` | `VARCHAR` | The physical dimension. Null if empty. |
| `physical_min` | `FLOAT` | The minimum value for the physical dimension. |
| `physical_max` | `FLOAT` | The maximum value for the physical dimension. |
| `digital_min` | `INTEGER` | The minimum value that can be recorded digitally. |
| `digital_max` | `INTEGER` | The maximum value that can be recorded digitally. |
| `prefiltering` | `VARCHAR` | The prefiltering applied to the signal. Null if empty. |
| `samples_per_records` | `UINTEGER` | The number of samples for each record for that signal. |
| `is_annotation` | `BOOLEAN` | Whether the signal contains annotations rather than digital recordings. |

### `read_edf_signal`

Import the time series data of EDF(+)/BDF(+) files into a table. Note that only non-annotation signals are included.

**Return schema:**

| Column Name | Data Type | Description |
|-------------|-----------|-------------|
| `filename` | `VARCHAR` | Path to the EDF(+)/BDF(+) file. |
| `record_id` | `UINTEGER` | The record ID, starting from 1 in the order of appearance. |
| `signal_id` | `UINTEGER` | The signal ID, starting from 1 in the order of appearance in the header. |
| `timestep` | `UINTEGER` | The time step, i.e. the number of previous samples starting from 0. |
| `time` | `DOUBLE` | The time since the start of the recording. |
| `digital_signal` | `INTEGER` | The recorded digital signal. |
| `physical_signal` | `FLOAT` | The digital signal converted into the physical dimension. |

### `read_edf_annotations`

Import the annotation data of EDF(+)/BDF(+) files into a table, one row per TAL (time-stamped annotation list). Note that
only annotation signals are included.

**Return schema:**

| Column Name | Data Type | Description |
|-------------|-----------|-------------|
| `filename` | `VARCHAR` | Path to the EDF(+)/BDF(+) file. |
| `record_id` | `UINTEGER` | The record ID, starting from 1 in the order of appearance. |
| `signal_id` | `UINTEGER` | The signal ID, starting from 1 in the order of appearance in the header. |
| `tal_id` | `UINTEGER` | The TAL ID, starting from 1 in the order of appearance. |
| `onset` | `DOUBLE` | The time since the recording start that the TAL is valid. |
| `duration` | `DOUBLE` | The length of time for which the TAL is valid. Null of empty. |
| `annotations` | `VARCHAR[]` | The TAL, as a list of annotations. |
