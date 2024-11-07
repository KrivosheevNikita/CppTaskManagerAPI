import unittest
import requests
import uuid

BASE_URL = "http://localhost:8080"
task_id = None
auth_token = None

headers = {
    "Content-Type": "application/json"
}


def test_login():
    global auth_token
    payload = {
        "username": "test_user",
        "password": "1234"
    }
    response = requests.post(f"{BASE_URL}/login", json=payload)
    assert response.status_code == 200

    json_data = response.json()
    assert "token" in json_data
    auth_token = json_data["token"]
    headers["Authorization"] = f"Bearer {auth_token}"


def test_login_wrong_password():
    global auth_token
    payload = {
        "username": "test_user",
        "password": "123"
    }
    response = requests.post(f"{BASE_URL}/login", json=payload)
    assert response.status_code == 401


def test_register_user():
    payload = {
        "username": f"{uuid.uuid4()}", 
        "email": f"{uuid.uuid4()}",
        "password": "1234"
    }
    response = requests.post(f"{BASE_URL}/register", json=payload)
    assert response.status_code == 200


def test_register_existing_user():
    payload = {
        "username": "test_user", 
        "email": "test@gmail.com",
        "password": "1234"
    }
    response = requests.post(f"{BASE_URL}/register", json=payload)
    assert response.status_code == 409


def test_create_task():
    global task_id
    task_name = f"Task {uuid.uuid4()}"
    payload = {
        "task_name": task_name,
        "description": "test",
        "priority": 2,
        "due_date": "2023-12-31"
    }
    response = requests.post(f"{BASE_URL}/tasks", json=payload, headers=headers)
    assert response.status_code == 200

    json_data = response.json()
    assert "task_id" in json_data
    task_id = json_data["task_id"]


def test_create_task_missing_fields():
    payload = {
        "description": "test",
        "priority": 2
    }
    response = requests.post(f"{BASE_URL}/tasks", json=payload, headers=headers)
    assert response.status_code == 400  


def test_get_task():
    global task_id
    response = requests.get(f"{BASE_URL}/tasks/{task_id}", headers=headers)
    assert response.status_code == 200

    json_data = response.json()
    assert json_data["task_id"] == task_id


def test_get_task_wrong_id():
    global task_id
    response = requests.get(f"{BASE_URL}/tasks/1000000000", headers=headers)
    assert response.status_code == 403


def test_update_task():
    global task_id
    task_name = f"Task {uuid.uuid4()}"
    updated_payload = {
        "task_name": task_name,
        "description": "test",
        "priority": 1,
        "due_date": "2024-01-01"
    }
    response = requests.put(f"{BASE_URL}/tasks/{task_id}", json=updated_payload, headers=headers)
    assert response.status_code == 200

    response = requests.get(f"{BASE_URL}/tasks/{task_id}", headers=headers)
    assert response.status_code == 200

    json_data = response.json()
    assert json_data["task_name"] == updated_payload["task_name"]
    assert json_data["description"] == updated_payload["description"]
    assert json_data["priority"] == updated_payload["priority"]
    assert json_data["due_date"] == updated_payload["due_date"]


def test_update_task_wrong_id():
    response = requests.put(f"{BASE_URL}/tasks/1000000000", headers=headers)
    assert response.status_code == 400


def test_get_all_tasks():
    response = requests.get(f"{BASE_URL}/tasks", headers=headers)
    assert response.status_code == 200
    json_data = response.json()
    assert "tasks" in json_data  


def test_add_tags_to_task():
    global task_id
    tags_payload = {"tags": ["tag1", "tag2"]}
    response = requests.post(f"{BASE_URL}/tasks/{task_id}/tags", json=tags_payload, headers=headers)
    assert response.status_code == 200

    response = requests.get(f"{BASE_URL}/tasks/{task_id}", headers=headers)
    assert response.status_code == 200

    json_data = response.json()
    assert "tags" in json_data
    assert set(tags_payload["tags"]).issubset(set(json_data["tags"]))


def test_delete_task():
    global task_id
    response = requests.delete(f"{BASE_URL}/tasks/{task_id}", headers=headers)
    assert response.status_code == 200

    response = requests.get(f"{BASE_URL}/tasks/{task_id}", headers=headers)
    assert response.status_code == 403


def test_delete_task_wrong_id():
    global task_id
    response = requests.delete(f"{BASE_URL}/tasks/1000000000", headers=headers)
    assert response.status_code == 404


if __name__ == "__main__":
    test_login()
    test_create_task()
    unittest.main()
