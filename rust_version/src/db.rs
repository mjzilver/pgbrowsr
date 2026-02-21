use sqlx::Column;
use sqlx::{PgPool, postgres::PgPoolOptions, Row, Postgres};
use sqlx::postgres::PgRow;

pub const DB_USER: &str = "botje";
pub const DB_PASSWORD: &str = "rata";
pub const DB_HOST: &str = "localhost";
pub const DB_PORT: u16 = 5432;
pub const DB_NAME: &str = "botdb";

pub async fn init_db() -> Result<PgPool, String> {
    let database_url = format!(
        "postgres://{}:{}@{}:{}/{}",
        DB_USER, DB_PASSWORD, DB_HOST, DB_PORT, DB_NAME
    );

    PgPoolOptions::new()
        .max_connections(5)
        .connect(&database_url)
        .await
        .map_err(|e| e.to_string())
}

pub async fn fetch_tables(pool: PgPool) -> Result<Vec<String>, String> {
    let rows = sqlx::query(
        r#"
        SELECT table_name
        FROM information_schema.tables
        WHERE table_schema = 'public'
        "#,
    )
    .fetch_all(&pool)
    .await
    .map_err(|e| e.to_string())?;

    let tables = rows
        .into_iter()
        .map(|row| row.get::<String, _>("table_name"))
        .collect();

    Ok(tables)
}

use crate::state::{TableDetails, ColumnDetails, RowDetails};

pub async fn fetch_table_details(pool: PgPool, table_name: String) -> Result<TableDetails, String> {
    let columns = sqlx::query(
        r#"
        SELECT column_name, data_type, is_nullable
        FROM information_schema.columns
        WHERE table_name = $1
        "#,
    )
    .bind(&table_name)
    .fetch_all(&pool)
    .await
    .map_err(|e| e.to_string())?
    .into_iter()
    .map(|row| ColumnDetails {
        name: row.get("column_name"),
        data_type: row.get("data_type"),
        is_nullable: row.get::<String, _>("is_nullable") == "YES",
    })
    .collect();

    let row_count = sqlx::query(&format!("SELECT COUNT(*) FROM {}", table_name))
        .fetch_one(&pool)
        .await
        .map_err(|e| e.to_string())?
        .get::<i64, _>(0);

    Ok(TableDetails {
        name: table_name,
        columns,
        row_count,
        rows: None,
    })
}

fn decode_or_null<T>(row: &PgRow, i: usize) -> String
where
    T: for<'r> sqlx::Decode<'r, Postgres> + sqlx::Type<Postgres> + ToString,
{
    match row.try_get::<Option<T>, _>(i) {
        Ok(Some(v)) => v.to_string(),
        _ => "NULL".to_string(),
    }
}

fn row_value_to_string(row: &PgRow, i: usize) -> String {
    let type_info_str = format!("{:?}", <PgRow as sqlx::Row>::column(row, i).type_info());

    if type_info_str.contains("INT2") {
        return decode_or_null::<i16>(row, i);
    }
    if type_info_str.contains("INT4") {
        return decode_or_null::<i32>(row, i);
    }
    if type_info_str.contains("INT8") {
        return decode_or_null::<i64>(row, i);
    }
    if type_info_str.contains("FLOAT4") {
        return decode_or_null::<f32>(row, i);
    }
    if type_info_str.contains("FLOAT8") {
        return decode_or_null::<f64>(row, i);
    }
    if type_info_str.contains("BOOL") {
        return decode_or_null::<bool>(row, i);
    }
    decode_or_null::<String>(row, i)
}

pub async fn fetch_top_rows(pool: PgPool, table_name: String) -> Result<Vec<RowDetails>, String> {
    let query = format!("SELECT * FROM {} LIMIT 30", table_name);
    let rows = sqlx::query(&query)
        .fetch_all(&pool)
        .await
        .map_err(|e| e.to_string())?;

    let result = rows
        .into_iter()
        .map(|row| {
            let values = (0..row.len())
                .map(|i| row_value_to_string(&row, i))
                .collect();
            RowDetails { values }
        })
        .collect();

    Ok(result)
}
