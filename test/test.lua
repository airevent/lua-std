--

local std = require "std"
local trace = require "trace"

--

trace(std)

-- print(std.finite(0)) -- true
-- print(std.finite(123)) -- true
-- print(std.finite(-1.5e10)) -- true
-- print(std.finite(0/0)) -- false
-- print(std.finite(1/0)) -- false
-- print(std.finite(-1/0)) -- false
-- print(std.finite(math.log(-1))) -- false
