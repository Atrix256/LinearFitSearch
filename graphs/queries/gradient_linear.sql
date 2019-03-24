.mode csv

select a.Samples, a.Average, b.Average, a.Minimum, a.Maximum, b.Minimum, b.Maximum from
	results as a inner join results as b on a.Samples = b.Samples
	where
		a.SearchFn = "Gradient" and a.DataDist = "Linear"
		and
		b.SearchFn = "Binary Search" and b.DataDist = "Linear";
