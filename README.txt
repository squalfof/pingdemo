
A DEMO.

All about design. Project could be split into 2 parts: 
-- load data into db
-- read data from db

1. load data
Basic: read data {key: value} from input file, insert {key, offset} into db, while mmap original input file,
    overwrite it into form like [value_len+value]. By using 'offset', we could directly fetch value from mmapped-file.
Inprovements: disable WAL/compaction during insertion, do Compaction after insertion;
Tons of stuff TBD: an abstract StorageEngine; value_len encoded in variable-len digits; 
    PlainFormat for SSTable; Encoder/Decoder; shut down stuff; UT;

2. read data
Basic: provide thrift service, expose Get() interface;
Search Strategy: sequential read is favorable given HDD, we could sacrifice some requests to acquire global optimum.
    We partition request into 12 segments, each segment contains value have nearby offset. We do the search stuff in a 
    round-robin fashion, through which sequential-read could be obtained. (Elevator-style)
Tons of stuff TBD: an abstract schedule-strategy; UT;

Others: change thrift simple server to non-blocking server; logs; confs;



