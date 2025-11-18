import json
import os
import sys
from google.oauth2 import service_account
from googleapiclient.discovery import build
from googleapiclient.http import MediaFileUpload


def get_drive_service():
    service_account_info = json.loads(os.environ["GOOGLE_SERVICE_ACCOUNT_JSON"])
    scopes = ["https://www.googleapis.com/auth/drive.file"]
    creds = service_account.Credentials.from_service_account_info(
        service_account_info,
        scopes=scopes,
    )
    service = build("drive", "v3", credentials=creds)
    return service


def upload_file(service, folder_id: str, file_path: str, file_name: str | None = None):
    if file_name is None:
        file_name = os.path.basename(file_path)

    media = MediaFileUpload(file_path, resumable=True)

    file_metadata = {
        "name": file_name,
        "parents": [folder_id],
    }

    file = (
        service.files()
        .create(
            body=file_metadata,
            media_body=media,
            fields="id, name, webViewLink, webContentLink",
        )
        .execute()
    )

    return file


def main():
    if len(sys.argv) < 2:
        print("usage: upload_to_drive.py <file_path> [<override_name>]")
        sys.exit(1)

    file_path = sys.argv[1]
    override_name = sys.argv[2] if len(sys.argv) >= 3 else None

    folder_id = os.environ["GOOGLE_DRIVE_FOLDER_ID"]

    if not os.path.exists(file_path):
        print(f"file not found: {file_path}", file=sys.stderr)
        sys.exit(1)

    service = get_drive_service()
    uploaded = upload_file(service, folder_id, file_path, override_name)

    print("=== Uploaded to Google Drive ===")
    print("ID          :", uploaded["id"])
    print("Name        :", uploaded["name"])
    print("webViewLink :", uploaded.get("webViewLink"))
    print("webContentLink :", uploaded.get("webContentLink"))


if __name__ == "__main__":
    main()
