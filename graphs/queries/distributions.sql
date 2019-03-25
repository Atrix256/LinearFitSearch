.mode csv

select a.Idx, a.Value, b.Value, c.Value, d.Value, e.Value, f.Value, g.Value from
	(select Idx, Value from samples where Dist="Linear") as a
	left join samples as b on a.Idx = b.Idx and b.Dist="Linear Outlier"
	left join samples as c on a.Idx = c.Idx and c.Dist="Log"
	left join samples as d on a.Idx = d.Idx and d.Dist="Cubic"
	left join samples as e on a.Idx = e.Idx and e.Dist="Quadratic"
	left join samples as f on a.Idx = f.Idx and f.Dist="Random"
	left join samples as g on a.Idx = g.Idx and g.Dist="Normal";
