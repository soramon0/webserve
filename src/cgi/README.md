CGI is a protocol that defines how the server will communicate with external programs

## Full Flow

- the browser sends an HTTP request to the server, since cgi script don't understand HTTP the server translates the request details into environment variables and passes them to the script
- CGI takes the environment variables and the request body (if it exists) as input, run the script and produce a raw output.
- the server then take this output and wraps it into a proper HTTP response that the browser can understand

---

## What I've done So far #

### Constructor

```cpp
CgiHandler(const std::string& scriptPath,
		const std::string& interpreter,
		const std::string& body,
		const std::map<std::string, std::string>& env);
```
For now the constructor takes these arguments :
- `_scriptPath` : the path of the script to execute
- `_interpreter` : the interpreter used to run the script 
- `_body` : the request body used as input for the script
- `_env` : a hardcoded map that contains environment variables the script needs, for now this is what I think I need :
	- `REQUEST_METHOD` => tells the script what HTTP method was used (GET/POST)
	- `CONTENT_TYPE` => needed for POST handling, tells how the request body is formatted
	- `QUERY_STRING` => needed for GET parameters
I might need more than these ofc but for now this is what I want

> In the future, the constructor will take just two parameters: a `Request` struct and a `Location` struct. the `Request` struct will contain everything needed to build the environment variables (we must agree on this later)

---

### `execute()`

```cpp
std::string	execute();
```

This the core function :
- opens two pipes `pipe_in` to write input to the script and `pipe_out` to read output from the script
- call `fork()` to create a child process in which the script will run
**inside the child I call** `childProcess()` handles execution:
- duplicates `stdin` and `stdout` to point to the pipes
- calls `chidr()` to set the working directory to the script's directory, this was required by the subject :
> *"The CGI should be run in the correct directory for relative path file access."*
- builds the argument list and evironment variables
- calls `execve()` which replaces the child process with the running script

**inside the parent I call** `parentProcess()` handles I/O:
- writes the request body to the child's `stdin` (if a the body exists)
- reads the script's output from `pipe_out` into a `std::string`
- waits for the child to finish
- returns the raw output

---

### `buildRequest()`

```c++
	std::string	buildResponse(const std::string& cgiOutput);
```
it takes the raw CGI output, splits it into headers and body at the `\r\n\r\n` separator then wraps everything into a proper HTTP response

---

## What's still left

- [ ] non-blocking pipe reads
- [ ] timeout handling
- [ ] cgi detection function (I think I'm responsible for this one)
- [ ] refactor constructor to take `Request` and `Location` structs
- [ ] full integration with the epoll loop
- [ ] testing edge cases

---

## Notes

1. the implementation is still in progress, but I'd appreciate your feedback
2. I tried to write a sub struct `Request` I'll need from the request parser and this is how it looks :

```cpp
struct Request
{
	std::string method;
	std::string path;
	std::string	queryString;
	std::string body;
	std::string contentType;
	std::string protocol;
};
```
this might change as we figure out the integration details together :)


