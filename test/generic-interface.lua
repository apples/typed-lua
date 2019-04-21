
interface List<T>: { [number]: T }

local horse: List<string>

horse = nil

interface Node<T>: { val: T; next: Node<T> }

local node: Node<string>

node = nil

node.next = nil
