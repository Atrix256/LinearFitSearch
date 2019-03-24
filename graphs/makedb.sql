.mode csv

CREATE TABLE results(
	  "DataDist" TEXT,
	  "SearchFn" TEXT,
	  "Samples" INTEGER,
	  "Minimum" INTEGER,
	  "Maximum" INTEGER,
	  "Avgerage" REAL,
	  "Single" INTEGER
);
.import results.csv results

CREATE TABLE samples(
	  "Dist" TEXT,
	  "Idx" INTEGER,
	  "Value" INTEGER
);
.import samples.csv samples
