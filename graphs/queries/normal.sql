.mode csv

select a.Samples, a.Average, b.Average, a.Minimum, a.Maximum, b.Minimum, b.Maximum from
	results as a inner join results as b on a.Samples = b.Samples
	where
		a.SearchFn = "Line Fit" and a.DataDist = "Normal"
		and
		b.SearchFn = "Binary Search" and b.DataDist = "Normal";
