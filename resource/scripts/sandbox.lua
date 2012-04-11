function table.copy(t)
  local u = { }
  for k, v in pairs(t) do u[k] = v end
  return setmetatable(u, getmetatable(t))
end

robot_env = table.copy(_G)
levelgen_env = table.copy(_G)

