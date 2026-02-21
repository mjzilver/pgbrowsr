#[derive(Debug, Clone)]
pub struct ColumnDetails {
    pub name: String,
    pub data_type: String,
    pub is_nullable: bool,
}

#[derive(Debug, Clone)]
pub struct RowDetails {
    pub values: Vec<String>,
}

#[derive(Debug, Clone)]
pub struct TableDetails {
    pub name: String,
    pub columns: Vec<ColumnDetails>,
    pub row_count: i64,
    pub rows: Option<Vec<RowDetails>>,
}

#[derive(Default)]
pub struct DatabaseViewer {
    pub pool: Option<sqlx::PgPool>,
    pub tables: Vec<String>,
    pub table_details: Option<TableDetails>,
    pub error: Option<String>,
    pub loading_tables: bool,
    pub loading_details: bool,
    pub loading_top_rows: bool,
}

#[derive(Debug, Clone)]
pub enum Message {
    InitDatabase,
    DatabaseInitialized(Result<sqlx::PgPool, String>),
    TablesLoaded(Result<Vec<String>, String>),
    ClickTable(String),
    TableDetailsLoaded(Result<TableDetails, String>),
    FetchTopRows(String),
    TopRowsLoaded(Result<Vec<RowDetails>, String>),
}
