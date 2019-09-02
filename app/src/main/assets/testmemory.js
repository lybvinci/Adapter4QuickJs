function fun() {
  return (this instanceof String);
}
console.log(fun());