# Task Manager API
Task Manager API is a C++ RESTful API for task management built using C++ and Crow framework. It uses PostgreSQL as a database and Redis as a cache. Authorization using a JWT token is used. Passwords in the database are stored in hashed form. Task Manager API allows users to create, update, view, and delete tasks, add tags for organization and filtering.
## Features

- User registration and login with token-based authentication (JWT)
- Create, update, delete, and view tasks
- Add tags to tasks
- Get all tasks associated with a specific user
## Technology Stack

- C++
- Crow framework
- PostgreSQL
- Redis
- pytest for tests


## API Endpoints

The following endpoints are available in the Task Manager API:

### Authentication
- `POST /register` — Register a new user
- `POST /login` — Log in and receive an authorization token

### Task Management
- `POST /tasks` — Create a new task
- `GET /tasks/{task_id}` — Receive a  task by ID
- `PUT /tasks/{task_id}` — Update an existing task
- `DELETE /tasks/{task_id}` — Delete a task
- `GET /tasks?tags={tag}` — Recieve all tasks for the authenticated user with optional filtering by tags
- `POST /tasks/{task_id}/tags` — Add tags to a task


## Usage

### Authentication
To access the API, include an authorization token in your request headers

#### `POST /register`
Registers a new user

**Request:**
```
POST /register
{
    "username": "user",
    "email": "user@gmail.com",
    "password": "password"
}
```

**Response:**
- **200**: User registered successfully
- **400**: Invalid or missing JSON data

---

#### `POST /login`
Authenticates a user and returns an authorization token

**Request:**
```
POST /login
{
    "username": "user",
    "password": "password"
}
```

**Response:**
- **200**: Returns the authorization token.
  ```
  {
      "token": "your_authorization_token"
  }
  ```
- **401**: Invalid username or password.

---

#### `POST /tasks`
Creates a new task for the authenticated user

**Request:**
```
POST /tasks
Headers: { "Authorization": "Bearer your_authorization_token" }
{
    "task_name": "New Task", 
    "description": "New Task",
    "priority": 1, (optional)
    "status_id": 2, (optional)
    "due_date": "2025-01-01", (optional)
    "tags": ["tag1", "tag2"] (optional)
}
```

**Response:**
- **200**: Task created successfully
  ```
  {
      "message": "Task was created successfully",
      "task_id": 1
  }
  ```
- **400**: Invalid or missing JSON data
- **401**: Missing or invalid authorization token

---

#### `PUT /tasks/{task_id}`
Updates a task

**Request:**
```
PUT /tasks/1
Headers: { "Authorization": "Bearer your_authorization_token" }
{
    "task_name": "New Task", 
    "description": "New Task",
    "priority": 1, 
    "status_id": 2, 
    "due_date": "2025-01-01", 
    "tags": ["tag1", "tag2"] 
}
```

**Response:**
- **200**: Task updated successfully
- **400**: Invalid or missing JSON data
- **401**: Missing or invalid authorization token.
- **403**: Access denied
---

#### `GET /tasks/{task_id}`
Recieves a task by ID

**Request:**
```
GET /tasks/1
Headers: { "Authorization": "Bearer your_authorization_token" }
```

**Response:**
- **200**: Returns task details
  ```
  {
    "task_id": 1,
    "task_name": "New Task", 
    "description": "New Task",
    "priority": 1, 
    "status_id": 2, 
    "due_date": "2025-01-01", 
    "tags": ["tag1", "tag2"] 
  }
  ```
- **401**: Missing or invalid authorization token
- **403**: Access denied

---

#### `GET /tasks`
Recieves a list of all tasks for the authenticated user

**Request:**
```
GET /tasks?tags=tag1,tag2
Headers: { "Authorization": "Bearer your_authorization_token" }
```

**Response:**
- **200**: Returns an array of all tasks with optional filtering by tags
  ```
  {
      "tasks": [
          {
            "task_id": 1,
            "task_name": "New Task", 
            "description": "New Task",
            "priority": 1, 
            "status_id": 2, 
            "due_date": "2025-01-01", 
            "tags": ["tag1", "tag2"] 
          }
      ]
  }
  ```
- **401**: Missing or invalid authorization token

---

#### `DELETE /tasks/{task_id}`
Deletes a task by ID

**Request:**
```
DELETE /tasks/1
Headers: { "Authorization": "Bearer your_authorization_token" }
```

**Response:**
- **200**: Task deleted successfully
- **401**: Missing or invalid authorization token 
- **404**: Task not found
---

#### `POST /tasks/{task_id}/tags`
Adds tags to an existing task

**Request:**
```
POST /tasks/1/tags
Headers: { "Authorization": "Bearer your_authorization_token" }
{
    "tags": ["new_tag1", "new_tag2"]
}
```

**Response:**
- **200**: Tags added successfully
- **400**: Invalid or missing JSON data
- **401**: Missing or invalid authorization token
- **403**: Access denied



