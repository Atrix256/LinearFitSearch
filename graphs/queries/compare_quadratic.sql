.mode csv

select a.Samples, a.Average, b.Average, a.Minimum, a.Maximum, b.Minimum, b.Maximum from
	results as a inner join results as b on a.Samples = b.Samples
	where
		a.SearchFn = "Gradient" and a.DataDist = "Cubic"
		and
		b.SearchFn = "Line Fit" and b.DataDist = "Cubic";
