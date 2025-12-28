# Gemini Project Rules

- **Pathing Rule**: No paths are allowed to be hardcoded or absolute. Utilize environment variables, configuration files, or relative pathing strategies for dynamic path resolution. Ensure all path references are resolved at runtime. If environment variables are not set but are needed, prompt the user for input.
- **Mandatory Rule**: Before writing code, I must search 'SuiteSpotv2.0\SuiteSpotDocuments\instructions' for relevant .md files. If none found, I must state so.
