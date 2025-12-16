# Mermaid Test

\htmlonly
<div class="mermaid">
flowchart TD
  A[Start] --> B{Condition}
  B -- Yes --> C[Do X]
  B -- No  --> D[Do Y]
  C --> E[End]
  D --> E[End]
</div>
\endhtmlonly

```mermaid
sequenceDiagram
  participant User
  participant System
  User->>System: Trigger event
  System-->>User: Acknowledge
```
